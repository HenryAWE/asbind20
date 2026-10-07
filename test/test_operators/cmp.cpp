#include <asbind_test/framework.hpp>
#include <asbind20/operators.hpp>

namespace test_operators
{
struct cmp_value
{
    int data = 0;

    friend std::strong_ordering operator<=>(const cmp_value& lhs, const cmp_value& rhs)
    {
        return lhs.data <=> rhs.data;
    }

    friend std::strong_ordering operator<=>(const cmp_value& lhs, int rhs)
    {
        return lhs.data <=> rhs;
    }
};

template <bool UseGeneric>
static void register_cmp_value(asbind20::engine_pointer engine)
{
    using namespace asbind20;

    constexpr int additional_flag = AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS;

    value_class<cmp_value, UseGeneric>(engine, "cmp_value", additional_flag)
        .behaviours_by_traits()
        .property("int data", &cmp_value::data)
        .use(const_this <=> const_this)
        .use(const_this <=> param<int>);

    auto* ti = engine->GetTypeInfoByName("cmp_value");
    ASSERT_THAT(ti, ::testing::NotNull());

    // Both cases register an "opCmp" method.
    std::size_t op_cmp_count = 0;
    for(AS_NAMESPACE_QUALIFIER asUINT i = 0; i < ti->GetMethodCount(); ++i)
    {
        auto* method = ti->GetMethodByIndex(i);
        ASSERT_THAT(method, ::testing::NotNull());

        if(std::string_view(method->GetName()) != "opCmp")
            continue;

        ++op_cmp_count;
        EXPECT_EQ(method->GetParamCount(), 1u);
        EXPECT_EQ(method->GetReturnTypeId(), AS_NAMESPACE_QUALIFIER asTYPEID_INT32);
    }

    EXPECT_EQ(op_cmp_count, 2u);
}

static void check_cmp_value(asbind20::engine_pointer engine)
{
    using namespace asbind20;

    auto* m = create_module(engine, "check_cmp_value");
    ASSERT_THAT(m, ::testing::NotNull());
    m->AddScriptSection(
        "check_cmp_value",
        // this <=> this
        "int this_this()\n"
        "{\n"
        "    cmp_value a; a.data = 1;\n"
        "    cmp_value b; b.data = 2;\n"
        "    int r = 0;\n"
        "    if( a < b ) ++r;\n"
        "    if( b > a ) ++r;\n"
        "    if( a <= b ) ++r;\n"
        "    if( a >= b ) ++r;\n"
        "    if( a < a ) ++r;\n"
        "    if( a <= a ) ++r;\n"
        "    return r;\n"
        "}\n"
        // this <=> param
        "int this_param()\n"
        "{\n"
        "    cmp_value a; a.data = 1;\n"
        "    int r = 0;\n"
        // The class type is the left operand
        "    if( a < 2 ) ++r;\n"
        "    if( a > 0 ) ++r;\n"
        "    if( a <= 1 ) ++r;\n"
        "    if( a >= 1 ) ++r;\n"
        "    if( a > 2 ) ++r;\n"
        "    if( a < 0 ) ++r;\n"
        // The class type is the right operand.
        // The engine swaps the operands and uses the same "opCmp".
        "    if( 2 > a ) ++r;\n"
        "    if( 0 < a ) ++r;\n"
        "    if( 1 <= a ) ++r;\n"
        "    if( 1 >= a ) ++r;\n"
        "    if( 0 > a ) ++r;\n"
        "    if( 2 < a ) ++r;\n"
        "    return r;\n"
        "}\n"
    );
    ASSERT_GE(m->Build(), 0);

    auto check = [&](const char* name, int expected)
    {
        auto* f = m->GetFunctionByName(name);
        ASSERT_THAT(f, ::testing::NotNull()) << name;

        request_context ctx(engine);
        auto result = script_invoke<int>(ctx, f);
        ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
        EXPECT_EQ(result.value(), expected) << name;
    };

    check("this_this", 4);
    check("this_param", 8);
}
} // namespace test_operators

TEST(Compare, Native)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine);

    test_operators::register_cmp_value<false>(engine.get());
    test_operators::check_cmp_value(engine.get());
}

TEST(Compare, Generic)
{
    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine);

    test_operators::register_cmp_value<true>(engine.get());
    test_operators::check_cmp_value(engine.get());
}
