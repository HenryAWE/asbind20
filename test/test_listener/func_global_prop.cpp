// Recording functions and global properties

#include <asbind20/asbind.hpp>
#include "listener_suites.hpp"

namespace test_listener
{
static int int_func()
{
    return 0;
}

static float float_func()
{
    return 0.0f;
}

static void gfn(asbind20::generic_pointer) {}

static int int_prop = 0;
static float float_prop = 0.0f;
} // namespace test_listener

using ListenerTest = test_listener::general_listener_suite;

namespace test_listener
{
template <bool UseGeneric>
static void test_record_functions(asbind20::engine_pointer engine)
{
    asbind20::global<UseGeneric, record_global> g(engine);
    auto& listener = g.get_listener();

    auto& mock = *listener.mock;
    EXPECT_CALL(mock, on_function("int_func")).Times(1);
    EXPECT_CALL(mock, on_function("float_func")).Times(1);
    EXPECT_CALL(mock, on_function("gfn")).Times(1);

    using asbind20::fp;
    g
        .function("int int_func()", fp<&test_listener::int_func>)
        .function("float float_func()", fp<&test_listener::float_func>)
        .function("void gfn()", &test_listener::gfn);
}
} // namespace test_listener

TEST_F(ListenerTest, RecordFunctionsGeneric)
{
    test_listener::test_record_functions<true>(engine);
}

TEST_F(ListenerTest, RecordFunctionsNative)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    test_listener::test_record_functions<false>(engine);
}

TEST_F(ListenerTest, RecordProperties)
{
    asbind20::global<true, test_listener::record_global> g(engine);
    auto& listener = g.get_listener();

    auto& mock = *listener.mock;
    EXPECT_CALL(mock, on_global_property("int_prop")).Times(1);
    EXPECT_CALL(mock, on_global_property("float_prop")).Times(1);

    g
        .property("int int_prop", test_listener::int_prop)
        .property("float float_prop", test_listener::float_prop);
}
