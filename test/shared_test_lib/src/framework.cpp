#include <asbind_test/framework.hpp>
#include <asbind_test/array.hpp>

namespace asbind_test
{
::testing::AssertionResult check_context_state(
    AS_NAMESPACE_QUALIFIER asEContextState state
)
{
    using asbind20::to_string;
    if(state == AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED)
        return ::testing::AssertionSuccess();

    return ::testing::AssertionFailure()
           << "state = " << to_string(state);
}

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
    asbind20::engine_pointer engine,
    bool propagate_error_to_gtest
)
{
    if(propagate_error_to_gtest)
    {
        asbind20::set_message_callback(
            engine, &message_callback_impl<true>
        );
    }
    else
    {
        asbind20::set_message_callback(
            engine, &message_callback_impl<false>
        );
    }
}

static void exception_translator_impl(
    asbind20::context_pointer ctx, void*
)
{
    ASSERT_NE(ctx, nullptr);

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
    asbind20::engine_pointer engine
)
{
    if(!asbind20::has_exceptions())
    {
        GTEST_LOG_(WARNING)
            << "has_exceptions() returned false, ignoring exception translator";
        return;
    }

    asbind20::set_exception_translator(
        engine, &exception_translator_impl
    );
}

void output_gc_statistics(
    std::ostream& os,
    const asbind20::debugging::gc_statistics& stat,
    char sep
)
{
    os << stat.description(std::string_view(&sep, 1));
    os << std::endl;
}

void output_gc_statistics(
    std::ostream& os,
    asbind20::engine_pointer engine,
    char sep
)
{
    auto stat = asbind20::debugging::get_gc_statistics(engine);
    output_gc_statistics(os, stat, sep);
}
} // namespace asbind_test
