// Recording functions and global properties

#include <asbind_test/framework.hpp>
#include <asbind20/asbind.hpp>
#include <gmock/gmock-matchers.h>
#include "listener_suites.hpp"

namespace test_listener
{
class record_func_global_prop
{
public:
    template <typename BindGenerator>
    void on_function(BindGenerator& g, int id)
    {
        if(id < 0)
        {
            using asbind20::to_string;
            GTEST_FAIL()
                << "bad function: " << to_string(static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(id));
        }

        asbind20::engine_pointer engine = g.get_engine();
        ASSERT_NE(engine, nullptr);
        auto* f = engine->GetFunctionById(id);
        ASSERT_NE(f, nullptr);

        recorded_func.emplace_back(f->GetName());
    }

    template <typename BindGenerator>
    void on_global_property(BindGenerator& g, int id)
    {
        using asbind20::to_string;
        if(id < 0)
        {
            GTEST_FAIL()
                << "bad global property: " << to_string(static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(id));
        }

        asbind20::engine_pointer engine = g.get_engine();
        ASSERT_NE(engine, nullptr);
        const char* name;
        int r = engine->GetGlobalPropertyByIndex(static_cast<AS_NAMESPACE_QUALIFIER asUINT>(id), &name);
        ASSERT_GE(r, 0)
            << "r = " << to_string(static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(r));
        ASSERT_NE(name, nullptr);

        recorded_prop.emplace_back(name);
    }

    template <typename BindGenerator>
    static void on_typedef(BindGenerator& g, int id)
    {
        GTEST_FAIL()
            << "Unreachable. name = " << g.get_name() << " id = " << id;
    }

    template <typename BindGenerator>
    static void on_funcdef(BindGenerator& g, int id)
    {
        GTEST_FAIL()
            << "Unreachable. name = " << g.get_name() << " id = " << id;
    }

    std::vector<std::string> recorded_func;
    std::vector<std::string> recorded_prop;
};

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

TEST_F(ListenerTest, RecordFunctions)
{
    asbind20::global<true, test_listener::record_func_global_prop> g(engine);
    auto& listener = g.get_listener();
    EXPECT_TRUE(listener.recorded_func.empty());

    using asbind20::fp;
    g
        .function("int int_func()", fp<&test_listener::int_func>)
        .function("float float_func()", fp<&test_listener::float_func>);

    EXPECT_THAT(listener.recorded_func, ::testing::SizeIs(2));
    EXPECT_THAT(listener.recorded_func, ::testing::Contains("int_func"));
    EXPECT_THAT(listener.recorded_func, ::testing::Contains("float_func"));

    EXPECT_THAT(listener.recorded_prop, ::testing::SizeIs(0));

    g.function("void gfn()", &test_listener::gfn);
    EXPECT_THAT(listener.recorded_func, ::testing::SizeIs(3));
    EXPECT_THAT(listener.recorded_func, ::testing::Contains("gfn"));
}

TEST_F(ListenerTest, RecordProperties)
{
    asbind20::global<true, test_listener::record_func_global_prop> g(engine);
    auto& listener = g.get_listener();
    EXPECT_TRUE(listener.recorded_prop.empty());

    g
        .property("int int_prop", test_listener::int_prop);
    EXPECT_THAT(listener.recorded_prop, ::testing::SizeIs(1));
    EXPECT_THAT(listener.recorded_prop, ::testing::Contains("int_prop"));

    g
        .property("float float_prop", test_listener::float_prop);
    EXPECT_THAT(listener.recorded_prop, ::testing::SizeIs(2));
    EXPECT_THAT(listener.recorded_prop, ::testing::Contains("float_prop"));

    EXPECT_THAT(listener.recorded_func, ::testing::SizeIs(0));
}
