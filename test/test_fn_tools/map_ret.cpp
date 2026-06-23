#include <gtest/gtest.h>
#include <asbind_test/framework.hpp>
#include <asbind20/asbind.hpp>

namespace test_fn_tools
{
static std::size_t return_sz(int a, int b)
{
    return a * 100 + b;
}

static void check_map_ret(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = engine->GetModule(
        "test", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );

    m->AddScriptSection(
        "test",
        "uint f() { return return_ui(10, 13); } "
    );
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByName("f");
    ASSERT_NE(f, nullptr);

    asbind20::request_context ctx(engine);
    auto result = asbind20::script_invoke<AS_NAMESPACE_QUALIFIER asUINT>(ctx, f);
    ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
    EXPECT_EQ(result.value(), 1013);
}
} // namespace test_fn_tools

TEST(MapRet, GlobalGeneric)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    global<true>(engine)
        .function(
            "uint return_ui(int a, int b)",
            fn_tools::map_ret<AS_NAMESPACE_QUALIFIER asUINT>(fp<&test_fn_tools::return_sz>)
        );

    test_fn_tools::check_map_ret(engine);
}

TEST(MapRet, GlobalNative)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    global(engine)
        .function(
            "uint return_ui(int a, int b)",
            fn_tools::map_ret<AS_NAMESPACE_QUALIFIER asUINT>(fp<&test_fn_tools::return_sz>)
        );

    test_fn_tools::check_map_ret(engine);
}
