#include <gtest/gtest.h>
#include <string>
#include <asbind_test/framework.hpp>
#include <asbind20/asbind.hpp>

namespace test_fn_tools
{
template <bool UseGeneric>
class test_fn_suite : public ::testing::Test
{
protected:
    void SetUp() override
    {
        if constexpr(!UseGeneric)
            ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

        m_engine = asbind20::make_script_engine();
        asbind_test::setup_message_callback(m_engine);
        asbind_test::setup_exception_translator(m_engine);
        asbind_test::setup_script_assertion(m_engine);
    }

    void TearDown() override
    {
        m_engine.reset();
    }

public:
    [[nodiscard]]
    asbind20::engine_pointer get_engine() const
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

// A non-trivially-destructible type. It is used to verify that the returned
// value is properly destroyed, even when the script declaration for the
// registered function is `void` (i.e. `map_ret<void>`).
struct map_ret_nontrivial
{
    static inline std::size_t ctor_count = 0;
    static inline std::size_t dtor_count = 0;
    static inline std::size_t live_count = 0;

    int value = 0;

    explicit map_ret_nontrivial(int val)
        : value(val)
    {
        ++ctor_count;
        ++live_count;
    }

    map_ret_nontrivial(const map_ret_nontrivial& other)
        : value(other.value)
    {
        ++ctor_count;
        ++live_count;
    }

    map_ret_nontrivial(map_ret_nontrivial&& other) noexcept
        : value(std::exchange(other.value, 0))
    {
        ++ctor_count;
        ++live_count;
    }

    map_ret_nontrivial& operator=(const map_ret_nontrivial&) = default;
    map_ret_nontrivial& operator=(map_ret_nontrivial&&) noexcept = default;

    ~map_ret_nontrivial()
    {
        ++dtor_count;
        --live_count;
    }

    static void reset_counts()
    {
        ctor_count = 0;
        dtor_count = 0;
        live_count = 0;
    }
};

static map_ret_nontrivial return_nontrivial(int val)
{
    return map_ret_nontrivial(val);
}

static std::string return_string()
{
    return "asbind20";
}

struct map_ret_test_helper
{
    std::size_t b = 0;

    map_ret_test_helper() = default;
    map_ret_test_helper(const map_ret_test_helper&) = default;

    map_ret_test_helper& operator=(const map_ret_test_helper&) = default;

    explicit map_ret_test_helper(int val)
        : b(val) {}

    std::size_t exchange_b(int new_b)
    {
        return std::exchange(b, new_b);
    }

    std::size_t return_sz_const(int a) const
    {
        return a * 100 + b;
    }

    map_ret_nontrivial return_nontrivial(int val)
    {
        return map_ret_nontrivial(val);
    }
};

static auto build_module(
    asbind20::engine_pointer engine, const char* code
)
{
    auto* m = asbind20::create_module(engine, "test_fn_tools");
    m->AddScriptSection(
        "test_fn_tools",
        code
    );
    EXPECT_GE(m->Build(), 0);
    return m;
}

static void check_map_ret(asbind20::engine_pointer engine)
{
    auto* m = build_module(
        engine,
        "uint f() { return return_ui(10, 13); }"
    );

    auto* f = m->GetFunctionByName("f");
    ASSERT_THAT(f, ::testing::NotNull());

    asbind20::request_context ctx(engine);
    auto result = asbind20::script_invoke<AS_NAMESPACE_QUALIFIER asUINT>(ctx, f);
    ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
    EXPECT_EQ(result.value(), 1013);
}

static void check_mfn_map_ret(asbind20::engine_pointer engine)
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
    ASSERT_THAT(f, ::testing::NotNull());

    asbind20::request_context ctx(engine);
    auto result = asbind20::script_invoke<AS_NAMESPACE_QUALIFIER asUINT>(ctx, f);
    ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
    EXPECT_EQ(result.value(), 1013);
}

static void check_map_ret_void_free(asbind20::engine_pointer engine)
{
    auto* m = build_module(
        engine,
        "void f() { discard_nontrivial(1013); }\n"
        "void g() { discard_string(); }\n"
    );

    auto* f = m->GetFunctionByName("f");
    ASSERT_THAT(f, ::testing::NotNull());

    map_ret_nontrivial::reset_counts();
    {
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<void>(ctx, f);
        ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
    }
    // The returned temporary must have been destroyed after the call.
    EXPECT_EQ(map_ret_nontrivial::live_count, 0);
    EXPECT_EQ(map_ret_nontrivial::ctor_count, 1);
    EXPECT_EQ(map_ret_nontrivial::dtor_count, map_ret_nontrivial::ctor_count);

    auto* g = m->GetFunctionByName("g");
    ASSERT_THAT(g, ::testing::NotNull());

    asbind20::request_context ctx(engine);
    auto result = asbind20::script_invoke<void>(ctx, g);
    ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
}

static void check_map_ret_void_mfn(asbind20::engine_pointer engine)
{
    auto* m = build_module(
        engine,
        "void f()\n"
        "{\n"
        "    helper h = helper(13);\n"
        "    h.discard_nontrivial(10);\n"
        "}"
    );

    auto* f = m->GetFunctionByName("f");
    ASSERT_THAT(f, ::testing::NotNull());

    map_ret_nontrivial::reset_counts();
    {
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<void>(ctx, f);
        ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
    }
    // The returned temporary must have been destroyed after the call.
    EXPECT_EQ(map_ret_nontrivial::live_count, 0);
    EXPECT_EQ(map_ret_nontrivial::ctor_count, 1);
    EXPECT_EQ(map_ret_nontrivial::dtor_count, map_ret_nontrivial::ctor_count);
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
        )
        .function(
            "void discard_nontrivial(int val)",
            fn_tools::map_ret<void>(fp<&test_fn_tools::return_nontrivial>)
        )
        .function(
            "void discard_string()",
            fn_tools::map_ret<void>(fp<&test_fn_tools::return_string>)
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
        )
        .method(
            "void discard_nontrivial(int val)",
            fn_tools::map_ret<void>(fp<&test_fn_tools::map_ret_test_helper::return_nontrivial>)
        );

    test_fn_tools::check_map_ret(engine);
    test_fn_tools::check_mfn_map_ret(engine);
    test_fn_tools::check_map_ret_void_free(engine);
    test_fn_tools::check_map_ret_void_mfn(engine);
}

TEST_F(FnWrapperTestNative, MapRet)
{
    using namespace asbind20;
    auto engine = get_engine();

    global(engine)
        .function(
            "uint return_ui(int a, int b)",
            fn_tools::map_ret<AS_NAMESPACE_QUALIFIER asUINT>(fp<&test_fn_tools::return_sz>)
        )
        .function(
            "void discard_nontrivial(int val)",
            fn_tools::map_ret<void>(fp<&test_fn_tools::return_nontrivial>)
        )
        .function(
            "void discard_string()",
            fn_tools::map_ret<void>(fp<&test_fn_tools::return_string>)
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
        )
        .method(
            "void discard_nontrivial(int val)",
            fn_tools::map_ret<void>(fp<&test_fn_tools::map_ret_test_helper::return_nontrivial>)
        );

    test_fn_tools::check_map_ret(engine);
    test_fn_tools::check_mfn_map_ret(engine);
    test_fn_tools::check_map_ret_void_free(engine);
    test_fn_tools::check_map_ret_void_mfn(engine);
}
