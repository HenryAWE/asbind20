#include <asbind_test/framework.hpp>
#include <asbind20/asbind.hpp>
#include <gmock/gmock-matchers.h>

namespace test_listener
{
class record_func
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

        recorded_func.push_back(f->GetName());
    }

    std::vector<std::string> recorded_func;
};

static int int_func()
{
    return 0;
}

static float float_func()
{
    return 0.0f;
}
} // namespace test_listener

TEST(GlobalListener, RecordFunctions)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);

    global<true, test_listener::record_func> g(engine);
    auto& listener = g.get_listener();
    EXPECT_TRUE(listener.recorded_func.empty());

    g
        .function("int int_func()", fp<&test_listener::int_func>)
        .function("float float_func()", fp<&test_listener::float_func>);

    EXPECT_THAT(listener.recorded_func, ::testing::SizeIs(2));
    EXPECT_THAT(listener.recorded_func, ::testing::Contains("int_func"));
    EXPECT_THAT(listener.recorded_func, ::testing::Contains("float_func"));
}
