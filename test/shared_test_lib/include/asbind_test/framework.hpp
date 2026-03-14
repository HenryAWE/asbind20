#pragma once

#include <gtest/gtest.h>
#include <typeinfo>
#include <source_location>
#include <asbind20/asbind.hpp>

// IWYU pragma: begin_exports

#include "assertion.hpp"
#include "std_string.hpp"
#include "test_exceptions.hpp"

// IWYU pragma: end_exports

template <typename Exception, typename... Args>
void asbind20::on_exception([[maybe_unused]] Args&&... args)
{
    [&]()
    {
        ADD_FAILURE()
            << "Exception name = " << typeid(Exception).name();
    }();

    std::terminate();
}

namespace asbind_test
{
::testing::AssertionResult check_context_state(
    AS_NAMESPACE_QUALIFIER asEContextState state
);

template <typename T>
::testing::AssertionResult result_has_value(const asbind20::script_invoke_result<T>& r)
{
    return check_context_state(r.error());
}

void setup_message_callback(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    bool propagate_error_to_gtest = false
);

void setup_exception_translator(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine
);

void output_gc_statistics(
    std::ostream& os,
    const asbind20::debugging::gc_statistics& stat,
    char sep = '\n'
);
void output_gc_statistics(
    std::ostream& os,
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    char sep = '\n'
);
} // namespace asbind_test

#define ASBIND_TEST_SKIP_IF_MAX_PORTABILITY()  \
    do                                         \
    {                                          \
        if(::asbind20::has_max_portability())  \
            GTEST_SKIP() << "MAX_PORTABILITY"; \
    } while(0)

#define ASBIND_TEST_EXPECT_INVOKE_RESULT(result)              \
    do                                                        \
    {                                                         \
        EXPECT_TRUE(::asbind_test::result_has_value(result)); \
    } while(0)

#define ASBIND_TEST_EXPECT_INVOKE_NO_RESULT(result)            \
    do                                                         \
    {                                                          \
        EXPECT_FALSE(::asbind_test::result_has_value(result)); \
    } while(0)

#define ASBIND_TEST_ASSERT_INVOKE_RESULT(result)              \
    do                                                        \
    {                                                         \
        ASSERT_TRUE(::asbind_test::result_has_value(result)); \
    } while(0)
