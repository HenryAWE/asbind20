#include <asbind_test/framework.hpp>
#include <asbind20/asbind.hpp>
#include <gmock/gmock-matchers.h>

namespace test_listener
{
class record_func_prop
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

        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine = g.get_engine();
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

        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine = g.get_engine();
        ASSERT_NE(engine, nullptr);
        const char* name;
        int r = engine->GetGlobalPropertyByIndex(static_cast<AS_NAMESPACE_QUALIFIER asUINT>(id), &name);
        ASSERT_GE(r, 0)
            << "r = " << to_string(static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(r));
        ASSERT_NE(name, nullptr);

        recorded_prop.emplace_back(name);
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

static int int_prop = 0;
static float float_prop = 0.0f;

class global_listener_suite : public ::testing::Test
{
public:
    void SetUp() override
    {
        engine = asbind20::make_script_engine();
        ASSERT_TRUE(engine);

        // Error will be reported by listeners
        asbind_test::setup_message_callback(engine, false);
    }

    void TearDown() override
    {
        engine.reset();
    }

    asbind20::script_engine engine;
};
} // namespace test_listener

using GlobalListener = test_listener::global_listener_suite;

TEST_F(GlobalListener, RecordFunctions)
{
    asbind20::global<true, test_listener::record_func_prop> g(engine);
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
}

TEST_F(GlobalListener, RecordProperties)
{
    asbind20::global<true, test_listener::record_func_prop> g(engine);
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
