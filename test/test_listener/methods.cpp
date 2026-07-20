#include <asbind20/asbind.hpp>
#include "listener_suites.hpp"

namespace test_listener
{
struct class_listener
{
    struct mock_method_listener
    {
        MOCK_METHOD(void, on_method, (std::string_view name), ());
    };

    std::shared_ptr<testing::StrictMock<mock_method_listener>> mock =
        std::make_shared<testing::StrictMock<mock_method_listener>>();
    std::string recorded_class;

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

        asbind20::engine_pointer engine = g.get_engine();
        ASSERT_NE(engine, nullptr);
        asbind20::typeinfo_pointer ti = engine->GetTypeInfoById(id);
        ASSERT_NE(ti, nullptr);

        EXPECT_THAT(recorded_class, ::testing::IsEmpty())
            << "Recording class for a second time";
        recorded_class = ti->GetName();
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

        asbind20::engine_pointer engine = g.get_engine();
        ASSERT_NE(engine, nullptr);
        auto* ti = engine->GetTypeInfoById(g.get_type_id());
        ASSERT_NE(ti, nullptr);
        auto* f = engine->GetFunctionById(id);
        ASSERT_NE(f, nullptr);

        EXPECT_EQ(f->GetObjectType(), ti)
            << "ti->GetName(): " << ti->GetName();

        mock->on_method(f->GetName());
    }
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
static void test_record_methods(asbind20::engine_pointer engine)
{
    using namespace asbind20;
    value_class<my_class, UseGeneric, class_listener> c(engine, "my_class");
    auto& listener = c.get_listener();
    EXPECT_EQ(listener.recorded_class, "my_class");

    auto& mock = *listener.mock;
    EXPECT_CALL(mock, on_method("foo")).Times(1);
    EXPECT_CALL(mock, on_method("bar")).Times(1);
    EXPECT_CALL(mock, on_method("opEquals")).Times(1);
    EXPECT_CALL(mock, on_method("opImplConv")).Times(1);
    EXPECT_CALL(mock, on_method("opConv")).Times(1);
    EXPECT_CALL(mock, on_method("gfn")).Times(1);
    EXPECT_CALL(mock, on_method("outer_func")).Times(1);

    c
        .method("void foo()", fp<&my_class::foo>)
        .method("void bar() const", fp<&my_class::bar>)
        .opEquals()
        .template opImplConv<int>()
        .template opConv<float>()
        .method("void gfn()", &gfn)
        .method("void outer_func(int)", fp<&outer_func>);
}
} // namespace test_listener

TEST_F(ListenerTest, RecordMethodsGeneric)
{
    test_listener::test_record_methods<true>(engine);
}

TEST_F(ListenerTest, RecordMethodsNative)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    test_listener::test_record_methods<false>(engine);
}
