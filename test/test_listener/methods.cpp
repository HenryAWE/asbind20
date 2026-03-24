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
};

class my_class
{
public:
    [[maybe_unused]]
    int placeholder;

    bool operator==(const my_class&) const = default;

    void foo() {}

    void bar() const {}
};
} // namespace test_listener

using ListenerTest = test_listener::general_listener_suite;

TEST_F(ListenerTest, RecordMethodsGeneric)
{
    using namespace asbind20;
    using test_listener::my_class, test_listener::class_listener;
    value_class<my_class, true, class_listener> c(engine, "my_class");
    auto& listener = c.get_listener();

    c
        .method("void foo()", fp<&my_class::foo>);
    EXPECT_THAT(listener.recorded_method, ::testing::SizeIs(1));
    EXPECT_THAT(listener.recorded_method, ::testing::Contains("foo"));

    c
        .method("void bar() const", fp<&my_class::bar>);
    EXPECT_THAT(listener.recorded_method, ::testing::SizeIs(2));
    EXPECT_THAT(listener.recorded_method, ::testing::Contains("bar"));

    c.opEquals();
    EXPECT_THAT(listener.recorded_method, ::testing::SizeIs(3));
    EXPECT_THAT(listener.recorded_method, ::testing::Contains("opEquals"));
}

TEST_F(ListenerTest, RecordMethodsNative)
{
    using namespace asbind20;
    if(has_max_portability())
        ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    using test_listener::my_class, test_listener::class_listener;
    value_class<my_class, false, class_listener> c(
        engine, "my_class", AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS
    );
    auto& listener = c.get_listener();

    c
        .method("void foo()", &my_class::foo);
    EXPECT_THAT(listener.recorded_method, ::testing::SizeIs(1));
    EXPECT_THAT(listener.recorded_method, ::testing::Contains("foo"));

    c
        .method("void bar() const", &my_class::bar);
    EXPECT_THAT(listener.recorded_method, ::testing::SizeIs(2));
    EXPECT_THAT(listener.recorded_method, ::testing::Contains("bar"));

    c.opEquals();
    EXPECT_THAT(listener.recorded_method, ::testing::SizeIs(3));
    EXPECT_THAT(listener.recorded_method, ::testing::Contains("opEquals"));
}
