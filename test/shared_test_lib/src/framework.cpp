#include <asbind_test/framework.hpp>
#include <asbind_test/array.hpp>

namespace asbind_test
{
template <bool PropagateError>
static void message_callback_impl(const AS_NAMESPACE_QUALIFIER asSMessageInfo* msg, void*)
{
#define ASBIND_TEST_MSG_CALLBACK_WRITE_SRC(lvl_str) \
    lvl_str << " (" << msg->section << ":" << msg->row << ":" << msg->col << "): "

    switch(msg->type)
    {
    case AS_NAMESPACE_QUALIFIER asMSGTYPE_ERROR:
        if constexpr(PropagateError)
        {
            FAIL()
                << ASBIND_TEST_MSG_CALLBACK_WRITE_SRC("ERROR")
                << msg->message;
            // FAIL() contains return statement
        }
        else
        {
            std::cerr
                << ASBIND_TEST_MSG_CALLBACK_WRITE_SRC("ERROR")
                << msg->message
                << std::endl;
            break;
        }

    case AS_NAMESPACE_QUALIFIER asMSGTYPE_WARNING:
        std::cerr
            << ASBIND_TEST_MSG_CALLBACK_WRITE_SRC("WARNING")
            << msg->message
            << std::endl;
        break;

    case AS_NAMESPACE_QUALIFIER asMSGTYPE_INFORMATION:
        std::cerr
            << ASBIND_TEST_MSG_CALLBACK_WRITE_SRC("INFO")
            << msg->message
            << std::endl;
    }

#undef ASBIND_TEST_MSG_CALLBACK_WRITE_SRC
}

void setup_message_callback(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    bool propagate_error_to_gtest
)
{
    if(propagate_error_to_gtest)
    {
        asbind20::global(engine)
            .message_callback(&message_callback_impl<true>);
    }
    else
    {
        asbind20::global(engine)
            .message_callback(&message_callback_impl<false>);
    }
}

static void exception_translator_impl(
    AS_NAMESPACE_QUALIFIER asIScriptContext* ctx, void*
)
{
#ifndef ASBIND20_NO_EXCEPTIONS
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

#else
    GTEST_FAIL()
        << "script exception at context "
        << static_cast<void*>(ctx);

#endif
}

void setup_exception_translator(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine
)
{
    if(!asbind20::has_exceptions())
        return;
    asbind20::global(engine)
        .exception_translator(&exception_translator_impl);
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
