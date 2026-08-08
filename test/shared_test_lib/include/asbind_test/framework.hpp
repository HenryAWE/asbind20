#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <typeinfo>
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

/**
 * @brief Setup script message callback
 *
 * @param engine Engine pointer. Cannot be nullptr
 * @param propagate_error_to_gtest True for triggering GTest failure if message level of ERROR is received
 */
void setup_message_callback(
    asbind20::engine_pointer engine,
    bool propagate_error_to_gtest = true
);

void setup_exception_translator(
    asbind20::engine_pointer engine
);
} // namespace asbind_test

#define ASBIND_TEST_SKIP_IF_MAX_PORTABILITY()           \
    do                                                  \
    {                                                   \
        if(::asbind20::has_max_portability())           \
            GTEST_SKIP() << "AS_MAX_PORTABILITY found"; \
    } while(0)

#define ASBIND_TEST_SKIP_IF_NO_THREADS()           \
    do                                             \
    {                                              \
        if(!::asbind20::has_threads())             \
            GTEST_SKIP() << "AS_NO_THREADS found"; \
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
