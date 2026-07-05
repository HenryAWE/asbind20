#include <asbind_test/framework.hpp>
#include <asbind20/asbind.hpp>
#include <gmock/gmock-matchers.h>
#include "listener_suites.hpp"

namespace test_listener
{
class class_listener
{
public:
    template <typename BindingGenerator>
    void on_class(BindingGenerator& g, int id)
    {
        if(id < 0)
        {
            using asbind20::to_string;
            GTEST_FAIL()
                << "Bad class type: " << to_string(static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(id));
        }

        SCOPED_TRACE("id = " + std::to_string(id));

        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine = g.get_engine();
        ASSERT_NE(engine, nullptr);
        asbind20::typeinfo_pointer ti = engine->GetTypeInfoById(id);
        ASSERT_NE(ti, nullptr);

        recorded_class.emplace_back(ti->GetName());
    }

    template <typename BindingGenerator>
    void on_method(BindingGenerator& g, int id)
    {
        if(id < 0)
        {
            using asbind20::to_string;
            GTEST_FAIL()
                << "Bad method: " << to_string(static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(id));
        }

        SCOPED_TRACE("id = " + std::to_string(id));

        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine = g.get_engine();
        ASSERT_NE(engine, nullptr);
        auto* ti = engine->GetTypeInfoById(g.get_type_id());
        ASSERT_NE(ti, nullptr);
        auto* f = engine->GetFunctionById(id);
        ASSERT_NE(f, nullptr);

        EXPECT_EQ(f->GetObjectType(), ti)
            << "ti.GetName(): " << ti->GetName();

        recorded_method.emplace_back(f->GetName());
    }

    std::vector<std::string> recorded_method;
    std::vector<std::string> recorded_class;
};

class my_class
{
public:
    [[maybe_unused]]
    int placeholder;

    bool operator==(const my_class&) const = default;

    void foo() {}

    void bar() const {}

    operator int() const
    {
        return 0;
    }

    explicit operator float() const
    {
        return 0;
    }
};

static void outer_func(
    [[maybe_unused]] my_class* this_, int
) {}

static void gfn(asbind20::generic_pointer) {}
} // namespace test_listener

using ListenerTest = test_listener::general_listener_suite;

namespace test_listener
{
template <bool UseGeneric>
static void test_record_method(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;
    value_class<my_class, true, class_listener> c(engine, "my_class");
    auto& listener = c.get_listener();
    EXPECT_THAT(listener.recorded_class, ::testing::Contains("my_class"));

    c
        .method("void foo()", fp<&my_class::foo>);
    EXPECT_THAT(listener.recorded_method, ::testing::SizeIs(1));
    EXPECT_THAT(listener.recorded_method, ::testing::Contains("foo"));

    c
        .method("void bar() const", fp<&my_class::bar>);
    EXPECT_THAT(listener.recorded_method, ::testing::SizeIs(2));
    EXPECT_THAT(listener.recorded_method, ::testing::Contains("bar"));

    c
        .opEquals()
        .opImplConv<int>();
    EXPECT_THAT(listener.recorded_method, ::testing::SizeIs(4));
    EXPECT_THAT(listener.recorded_method, ::testing::Contains("opEquals"));
    EXPECT_THAT(listener.recorded_method, ::testing::Contains("opImplConv"));

    c
        .opConv<float>();
    EXPECT_THAT(listener.recorded_method, ::testing::SizeIs(5));
    EXPECT_THAT(listener.recorded_method, ::testing::Contains("opConv"));

    c
        .method("void gfn()", &gfn)
        .method("void outer_func(int)", fp<&outer_func>);
    EXPECT_THAT(listener.recorded_method, ::testing::SizeIs(7));
    EXPECT_THAT(listener.recorded_method, ::testing::Contains("gfn"));
    EXPECT_THAT(listener.recorded_method, ::testing::Contains("outer_func"));
}
} // namespace test_listener

TEST_F(ListenerTest, RecordMethodsGeneric)
{
    test_listener::test_record_method<true>(engine);
}

TEST_F(ListenerTest, RecordMethodsNative)
{
    if(asbind20::has_max_portability())
        ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    test_listener::test_record_method<false>(engine);
}
