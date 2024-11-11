#include "shared.hpp"
#include <asbind20/ext/vocabulary.hpp>
#include <asbind20/ext/array.hpp>
#include <asbind20/ext/stdstring.hpp>
#include <asbind20/ext/assert.hpp>
#include <asbind20/ext/math.hpp>
#include <asbind20/builder.hpp>

namespace asbind_test
{
void msg_callback(const asSMessageInfo* msg, void*)
{
    if(msg->type == asEMsgType::asMSGTYPE_ERROR)
    {
        FAIL()
            << msg->section
            << "(" << msg->row << ':' << msg->col << "): "
            << msg->message;
    }
}

static void assert_callback(std::string_view sv)
{
    asIScriptContext* ctx = asGetActiveContext();

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
    m_engine = asCreateScriptEngine();
    m_engine->SetMessageCallback(asFUNCTION(msg_callback), this, asCALL_CDECL);

    m_engine->SetEngineProperty(asEP_USE_CHARACTER_LITERALS, true);

    register_all();
}

void asbind_test_suite::TearDown()
{
    m_engine->ShutDownAndRelease();
    m_engine = nullptr;
}

void asbind_test_suite::run_file(
    const std::filesystem::path& filename,
    const char* entry_decl
)
{
    if(!std::filesystem::exists(filename))
        FAIL() << "File not found: " << filename;

    asIScriptModule* m = m_engine->GetModule("run_file", asGM_ALWAYS_CREATE);

    int r = asbind20::load_file(
        m,
        filename
    );
    if(r < 0)
        FAIL() << "Failed to load " << filename << ", r = " << r;
    r = m->Build();
    if(r < 0)
        FAIL() << "Failed to build, r = " << r;

    asIScriptFunction* entry = m->GetFunctionByDecl(entry_decl);
    if(!entry)
        FAIL() << "Entry not found, decl = " << entry_decl;

    asIScriptContext* ctx = m_engine->CreateContext();
    auto run_file_result = asbind20::script_invoke<void>(ctx, entry);

    if(!run_file_result.has_value() && run_file_result.error() == asEXECUTION_EXCEPTION)
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
        .function<&test_print>("void print(const string &in msg)");
}

void asbind_test_suite_generic::register_all()
{
    using namespace asbind20;

    asIScriptEngine* engine = get_engine();

    ext::register_script_optional(engine);
    ext::register_script_array(engine);
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
        .function<&test_print>(use_generic, "void print(const string &in msg)");
}
} // namespace asbind_test
