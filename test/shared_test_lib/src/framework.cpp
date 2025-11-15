#include <asbind_test/framework.hpp>
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

static void exception_translator(AS_NAMESPACE_QUALIFIER asIScriptContext* ctx, void*)
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
        ctx->SetException("...");
    }
}

void setup_exception_translator(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine
)
{
    if(!asbind20::has_exceptions())
        return;
    asbind20::global(engine)
        .exception_translator(&exception_translator);
}

void output_gc_statistics(
    std::ostream& os,
    const asbind20::debugging::gc_statistics& stat,
    char sep
)
{
    os << "current: " << stat.current_size << sep;
    os << "total destroyed: " << stat.total_destroyed << sep;
    os << "total detected: " << stat.total_detected << sep;
    os << "new objects: " << stat.new_objects << sep;
    os << "total new destroyed: " << stat.total_new_destroyed;
    os << std::endl;
}

void output_gc_statistics(
    std::ostream& os,
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    char sep
)
{
    auto stat = asbind20::debugging::get_gc_statistics(engine);
    output_gc_statistics(os, stat, sep);
}
} // namespace asbind_test
