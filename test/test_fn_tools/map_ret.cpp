#include <gtest/gtest.h>
#include <asbind_test/framework.hpp>
#include <asbind20/asbind.hpp>

namespace test_fn_tools
{
template <bool UseGeneric>
class test_fn_suite : public ::testing::Test
{
public:
    void SetUp() override
    {
        if constexpr(!UseGeneric)
            ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

        m_engine = asbind20::make_script_engine();
        asbind_test::setup_message_callback(m_engine, true);
        asbind_test::setup_exception_translator(m_engine);
        asbind_test::setup_script_assertion(m_engine);
    }

    void TearDown() override
    {
        m_engine.reset();
    }

    auto get_engine() const
        -> AS_NAMESPACE_QUALIFIER asIScriptEngine*
    {
        return m_engine.get();
    }

private:
    asbind20::script_engine m_engine;
};

static std::size_t return_sz(int a, int b)
{
    return a * 100 + b;
}

struct map_ret_test_helper
{
    std::size_t b = 0;

    std::size_t exchange_b(int new_b)
    {
        return std::exchange(b, new_b);
    }

    std::size_t return_sz_const(int a) const
    {
        return a * 100 + b;
    }
};

static auto build_module(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, const char* code
)
{
    auto* m = engine->GetModule(
        "test_fn_tools", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    m->AddScriptSection(
        "test_fn_tools",
        code
    );
    EXPECT_GE(m->Build(), 0);
    return m;
}

static void check_map_ret(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = build_module(
        engine,
        "uint f() { return return_ui(10, 13); }"
    );

    auto* f = m->GetFunctionByName("f");
    ASSERT_NE(f, nullptr);

    asbind20::request_context ctx(engine);
    auto result = asbind20::script_invoke<AS_NAMESPACE_QUALIFIER asUINT>(ctx, f);
    ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
    EXPECT_EQ(result.value(), 1013);
}

static void check_mfn_map_ret(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = build_module(
        engine,
        "uint f()\n"
        "{\n"
        "    helper h = helper(13);\n"
        "    assert(h.b == 13);\n"
        "    return h.return_ui_const(10);\n"
        "}"
    );

    auto* f = m->GetFunctionByName("f");
    ASSERT_NE(f, nullptr);

    asbind20::request_context ctx(engine);
    auto result = asbind20::script_invoke<AS_NAMESPACE_QUALIFIER asUINT>(ctx, f);
    ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
    EXPECT_EQ(result.value(), 1013);
}
} // namespace test_fn_tools

using FnWrapperTestGeneric = test_fn_tools::test_fn_suite<true>;
using FnWrapperTestNative = test_fn_tools::test_fn_suite<false>;

TEST_F(FnWrapperTestGeneric, MapRet)
{
    using namespace asbind20;
    auto engine = get_engine();

    global<true>(engine)
        .function(
            "uint return_ui(int a, int b)",
            fn_tools::map_ret<AS_NAMESPACE_QUALIFIER asUINT>(fp<&test_fn_tools::return_sz>)
        );
    value_class<test_fn_tools::map_ret_test_helper, true>(
        engine,
        "helper",
        AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS | AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_MORE_CONSTRUCTORS
    )
        .behaviours_by_traits(use_generic)
        .constructor<int>("int b")
        .property("const int b", &test_fn_tools::map_ret_test_helper::b)
        .method(
            "uint return_ui_const(int a)const",
            fn_tools::map_ret<AS_NAMESPACE_QUALIFIER asUINT>(fp<&test_fn_tools::map_ret_test_helper::return_sz_const>)
        );

    test_fn_tools::check_map_ret(engine);
    test_fn_tools::check_mfn_map_ret(engine);
}

TEST_F(FnWrapperTestNative, MapRet)
{
    using namespace asbind20;
    auto engine = get_engine();

    global(engine)
        .function(
            "uint return_ui(int a, int b)",
            fn_tools::map_ret<AS_NAMESPACE_QUALIFIER asUINT>(fp<&test_fn_tools::return_sz>)
        );
    value_class<test_fn_tools::map_ret_test_helper>(
        engine,
        "helper",
        AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS | AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_MORE_CONSTRUCTORS
    )
        .behaviours_by_traits(use_generic)
        .constructor<int>("int b")
        .property("const int b", &test_fn_tools::map_ret_test_helper::b)
        .method(
            "uint return_ui_const(int a)const",
            fn_tools::map_ret<AS_NAMESPACE_QUALIFIER asUINT>(fp<&test_fn_tools::map_ret_test_helper::return_sz_const>)
        );

    test_fn_tools::check_map_ret(engine);
    test_fn_tools::check_mfn_map_ret(engine);
}
