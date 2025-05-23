#include <shared_test_lib.hpp>
#include <asbind20/ext/vocabulary.hpp>
#include <asbind20/ext/array.hpp>
#include <asbind20/ext/stdstring.hpp>
#include <asbind20/ext/assert.hpp>
#include <asbind20/ext/math.hpp>
#include <asbind20/ext/exec.hpp>

namespace asbind_test
{
void setup_message_callback(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    bool propagate_error_to_gtest
)
{
    if(propagate_error_to_gtest)
    {
        asbind20::global(engine)
            .message_callback(
                +[](const AS_NAMESPACE_QUALIFIER asSMessageInfo* msg, void*)
                {
#define ASBIND_TEST_MSG_CALLBACK_WRITE_SRC() \
    " (" << msg->section                     \
         << ":" << msg->row                  \
         << ":" << msg->col << "): "
                    switch(msg->type)
                    {
                    case AS_NAMESPACE_QUALIFIER asMSGTYPE_ERROR:
                        FAIL() << "ERROR" << ASBIND_TEST_MSG_CALLBACK_WRITE_SRC() << msg->message;
                        // FAIL() contains return statement

                    case AS_NAMESPACE_QUALIFIER asMSGTYPE_WARNING:
                        std::cerr << "WARNING" << ASBIND_TEST_MSG_CALLBACK_WRITE_SRC();
                        break;
                    case AS_NAMESPACE_QUALIFIER asMSGTYPE_INFORMATION:
                        std::cerr << "INFO" << ASBIND_TEST_MSG_CALLBACK_WRITE_SRC();
                    }
                    std::cerr << msg->message << std::endl;

#undef ASBIND_TEST_MSG_CALLBACK_WRITE_SRC
                }
            );
    }
    else
    {
        asbind20::global(engine)
            .message_callback(
                +[](const AS_NAMESPACE_QUALIFIER asSMessageInfo* msg, void*)
                {
                    switch(msg->type)
                    {
                    case AS_NAMESPACE_QUALIFIER asMSGTYPE_ERROR:
                        std::cerr << "ERROR: ";
                        break;
                    case AS_NAMESPACE_QUALIFIER asMSGTYPE_WARNING:
                        std::cerr << "WARNING: ";
                        break;
                    case AS_NAMESPACE_QUALIFIER asMSGTYPE_INFORMATION:
                        std::cerr << "INFO: ";
                    }
                    std::cerr << msg->message << std::endl;
                }
            );
    }
}

void setup_exception_translator(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine
)
{
    asbind20::global(engine)
        .exception_translator(
            +[](AS_NAMESPACE_QUALIFIER asIScriptContext* ctx, void*)
            {
                try
                {
                    throw;
                }
                catch(std::exception& e)
                {
                    ctx->SetException(e.what());
                }
                catch(...)
                {
                    ctx->SetException("Caught unknown exception");
                }
            }
        );
}

void asbind_test_suite::msg_callback(const AS_NAMESPACE_QUALIFIER asSMessageInfo* msg)
{
    if(msg->type == AS_NAMESPACE_QUALIFIER asMSGTYPE_ERROR)
    {
        FAIL()
            << msg->section
            << "(" << msg->row << ':' << msg->col << "): "
            << msg->message;
    }
    else if(msg->type == AS_NAMESPACE_QUALIFIER asMSGTYPE_WARNING)
    {
        std::cerr
            << msg->section
            << "(" << msg->row << ':' << msg->col << "): "
            << msg->message
            << std::endl;
    }
}

void asbind_test_suite::ex_translator(AS_NAMESPACE_QUALIFIER asIScriptContext* ctx)
{
    try
    {
        throw;
    }
    catch(const std::exception& e)
    {
        ctx->SetException(e.what());
    }
    catch(...)
    {
        ctx->SetException("Unknown exception");
    }
}

static void assert_callback(std::string_view sv)
{
    auto* ctx = asbind20::current_context();

    const char* section;
    int line = ctx->GetLineNumber(0, nullptr, &section);

    GTEST_FAIL_AT(section, line)
        << "Script assert() failed: " << sv;
}

static void test_print(const std::string& msg)
{
    std::cerr << msg << std::endl;
}

void asbind_test_suite::SetUp()
{
    using namespace asbind20;

    m_engine = make_script_engine();
    global g(m_engine);
    g
        .message_callback(&asbind_test_suite::msg_callback, *this)
        .exception_translator(&asbind_test_suite::ex_translator, *this);

    ext::configure_engine_for_ext_string(m_engine);

    register_all();
}

void asbind_test_suite::TearDown()
{
    m_engine.reset();
}

void asbind_test_suite::run_file(
    const std::filesystem::path& filename,
    const char* entry_decl
)
{
    if(!std::filesystem::exists(filename))
        FAIL() << "File not found: " << filename;

    auto* m = m_engine->GetModule(
        "run_file", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );

    int r = asbind20::ext::load_file(
        m,
        filename
    );
    if(r < 0)
        FAIL() << "Failed to load " << filename << ", r = " << r;
    r = m->Build();
    if(r < 0)
        FAIL() << "Failed to build, r = " << r;

    auto* entry = m->GetFunctionByDecl(entry_decl);
    if(!entry)
        FAIL() << "Entry not found, decl = " << entry_decl;

    auto* ctx = m_engine->CreateContext();
    auto run_file_result = asbind20::script_invoke<void>(ctx, entry);

    if(!run_file_result.has_value() &&
       run_file_result.error() == AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION)
    {
        int column;
        const char* section;
        int line = ctx->GetExceptionLineNumber(&column, &section);
        FAIL()
            << "Script exception at " << section << " (" << line << ':' << column << "): "
            << ctx->GetExceptionString();
    }
    else
        EXPECT_TRUE(result_has_value(run_file_result));

    ctx->Release();
    m->Discard();
}

void asbind_test_suite::register_all()
{
    using namespace asbind20;

    ext::register_script_optional(m_engine);
    ext::register_script_array(m_engine);
    ext::register_script_char(m_engine);
    ext::register_std_string(m_engine, true);
    ext::register_string_utils(m_engine);
    ext::register_math_constants(m_engine);
    ext::register_math_function(m_engine);
    ext::register_script_assert(
        m_engine,
        &assert_callback,
        false, // The exception will be set by GTEST_FAIL_AT() in the callback
        &ext::string_factory::get()
    );

    global(m_engine)
        .function(use_generic, "void print(const string&in msg)", fp<&test_print>);
}

void asbind_test_suite_generic::register_all()
{
    using namespace asbind20;

    auto* engine = get_engine();

    ext::register_script_optional(engine, true);
    ext::register_script_array(engine, true, true);
    ext::register_script_char(engine, true);
    ext::register_std_string(engine, true, true);
    ext::register_string_utils(engine, true);
    ext::register_math_constants(engine);
    ext::register_math_function(engine, true);
    ext::register_script_assert(
        engine,
        &assert_callback,
        false, // The exception will be set by GTEST_FAIL_AT() in the callback
        &ext::string_factory::get()
    );

    global(engine)
        .function(use_generic, "void print(const string &in msg)", fp<&test_print>);
}
} // namespace asbind_test
