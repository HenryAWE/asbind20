#include <asbind_test/framework.hpp>
#include <asbind20/operators.hpp>

namespace test_operators
{
struct eq_value
{
    int first = 0;
    int second = 0;

    // this == this
    bool operator==(const eq_value& rhs) const
    {
        return first == rhs.first && second == rhs.second;
    }

    // this == param
    friend bool operator==(const eq_value& lhs, int rhs)
    {
        return lhs.first + lhs.second == rhs;
    }
};

template <bool UseGeneric>
static void register_eq_value(asbind20::engine_pointer engine)
{
    using namespace asbind20;

    constexpr int additional_flag = AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS;

    value_class<eq_value, UseGeneric>(engine, "eq_value", additional_flag)
        .behaviours_by_traits()
        .property("int first", &eq_value::first)
        .property("int second", &eq_value::second)
        .use(const_this == const_this)
        .use(const_this == param<int>);

    auto* ti = engine->GetTypeInfoByName("eq_value");
    ASSERT_THAT(ti, ::testing::NotNull());

    // Both cases register an "opEquals" method.
    std::size_t op_equals_count = 0;
    for(AS_NAMESPACE_QUALIFIER asUINT i = 0; i < ti->GetMethodCount(); ++i)
    {
        auto* method = ti->GetMethodByIndex(i);
        ASSERT_THAT(method, ::testing::NotNull());

        if(std::string_view(method->GetName()) != "opEquals")
            continue;

        ++op_equals_count;
        EXPECT_EQ(method->GetParamCount(), 1u);
        EXPECT_EQ(method->GetReturnTypeId(), AS_NAMESPACE_QUALIFIER asTYPEID_BOOL);
    }

    EXPECT_EQ(op_equals_count, 2u);
}

static void check_eq_value(asbind20::engine_pointer engine)
{
    using namespace asbind20;

    auto* m = create_module(engine, "check_eq_value");
    ASSERT_THAT(m, ::testing::NotNull());
    m->AddScriptSection(
        "check_eq_value",
        "bool this_this()\n"
        "{\n"
        "    eq_value a; a.first = 1; a.second = 2;\n"
        "    eq_value b; b.first = 1; b.second = 2;\n"
        "    eq_value c; c.first = 1; c.second = 3;\n"
        "    return (a == b) && (a != c);\n"
        "}\n"
        "bool this_param()\n"
        "{\n"
        "    eq_value a; a.first = 1; a.second = 2;\n"
        // The class type is the left operand.
        // The class type is the right operand here.
        // The engine swaps the operands and uses the same "opEquals".
        "    return (a == 3) && (3 == a) && (a != 4) && (4 != a);\n"
        "}\n"
    );
    ASSERT_GE(m->Build(), 0);

    auto check = [&](const char* name)
    {
        auto* f = m->GetFunctionByName(name);
        ASSERT_THAT(f, ::testing::NotNull()) << name;

        request_context ctx(engine);
        auto result = script_invoke<bool>(ctx, f);
        ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
        EXPECT_TRUE(result.value()) << name;
    };

    check("this_this");
    check("this_param");
}
} // namespace test_operators

TEST(Equals, Native)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine);

    test_operators::register_eq_value<false>(engine.get());
    test_operators::check_eq_value(engine.get());
}

TEST(Equals, Generic)
{
    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine);

    test_operators::register_eq_value<true>(engine.get());
    test_operators::check_eq_value(engine.get());
}
