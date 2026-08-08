#include <asbind_test/framework.hpp>
#include <asbind20/asbind.hpp>

namespace test_bind
{
struct opCmp_value
{
    int data = 0;

    int cmp(const opCmp_value& other) const
    {
        return asbind20::translate_three_way(
            this->data <=> other.data
        );
    }
};

static int cmp_free(const opCmp_value& lhs, const opCmp_value& rhs)
{
    return lhs.cmp(rhs);
}

// Custom comparators for opCmp must return int.
// This covers the member function, free function and lambda forms.
template <bool UseGeneric>
static void register_opCmp_check_helpers(asbind20::engine_pointer engine)
{
    using namespace asbind20;

    constexpr int additional_flag = AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS;

    value_class<opCmp_value, UseGeneric>(engine, "cmp_member", additional_flag)
        .behaviours_by_traits()
        .property("int data", &opCmp_value::data)
        .opCmp(fp<&opCmp_value::cmp>);

    value_class<opCmp_value, UseGeneric>(engine, "cmp_free", additional_flag)
        .behaviours_by_traits()
        .property("int data", &opCmp_value::data)
        .opCmp(fp<&test_bind::cmp_free>);

    value_class<opCmp_value, UseGeneric>(engine, "cmp_lambda", additional_flag)
        .behaviours_by_traits()
        .property("int data", &opCmp_value::data)
        .opCmp(
            [](const opCmp_value& lhs, const opCmp_value& rhs) -> int
            {
                return lhs.cmp(rhs);
            }
        );
}

static void check_opCmp(asbind20::engine_pointer engine)
{
    using namespace asbind20;

    auto* m = create_module(engine, "check_opCmp");
    ASSERT_THAT(m, ::testing::NotNull());
    m->AddScriptSection(
        "check_opCmp",
        "int f()\n"
        "{\n"
        "    cmp_member a1; cmp_member b1;\n"
        "    a1.data = 1; b1.data = 2;\n"
        "    cmp_free a2; cmp_free b2;\n"
        "    a2.data = 1; b2.data = 2;\n"
        "    cmp_lambda a3; cmp_lambda b3;\n"
        "    a3.data = 1; b3.data = 2;\n"
        // All should be true
        "    return (a1 < b1 ? 1 : 0) + (b1 > a1 ? 1 : 0)\n"
        "         + (a2 < b2 ? 1 : 0) + (b2 > a2 ? 1 : 0)\n"
        "         + (a3 < b3 ? 1 : 0) + (b3 > a3 ? 1 : 0);\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByDecl("int f()");
    ASSERT_THAT(f, ::testing::NotNull());

    request_context ctx(engine);
    auto result = script_invoke<int>(ctx, f);
    ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
    EXPECT_EQ(result.value(), 6);
}
} // namespace test_bind

TEST(OpCmp, Native)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine);

    test_bind::register_opCmp_check_helpers<false>(engine);
    test_bind::check_opCmp(engine);
}

TEST(OpCmp, Generic)
{
    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine);

    test_bind::register_opCmp_check_helpers<true>(engine);
    test_bind::check_opCmp(engine);
}
