#include <asbind20/asbind.hpp>
#include "listener_suites.hpp"

using ListenerTest = test_listener::general_listener_suite;

TEST_F(ListenerTest, RecordTypedefs)
{
    asbind20::global<true, test_listener::record_global> g(engine);
    auto& listener = g.get_listener();

    auto& mock = *listener.mock;
    EXPECT_CALL(mock, on_typedef("float")).Times(1);
    EXPECT_CALL(mock, on_typedef("int64")).Times(1);

    g.typedef_("float", "flt32");
    g.using_("my_int", "int64");
}

TEST_F(ListenerTest, RecordFuncdefs)
{
    asbind20::global<true, test_listener::record_global> g(engine);
    auto& listener = g.get_listener();

    auto& mock = *listener.mock;
    EXPECT_CALL(mock, on_funcdef("foo")).Times(1);
    EXPECT_CALL(mock, on_funcdef("bar")).Times(1);

    g.funcdef("int foo(float)");
    g.funcdef("float bar(int)");
}
