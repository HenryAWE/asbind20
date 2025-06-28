#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/exec.hpp>

using namespace asbind_test;

namespace test_bind
{
static void set_int(int& out)
{
    out = 1013;
}

static int ASBIND20_STDCALL stdcall_func1(int a, float b)
{
    return a * 10 + int(b);
}

static int ASBIND20_STDCALL stdcall_func2(int a, float b)
{
    return a * int(b);
}

struct class_wrapper
{
    int value = 0;

    void set_val(int val)
    {
        value = val;
    }
};

// AngelScript declaration: int from_aux()
static void from_aux(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
{
    auto val = std::bit_cast<std::intptr_t>(gen->GetAuxiliary());

    asbind20::set_generic_return<int>(gen, static_cast<int>(val));
}
} // namespace test_bind

static void register_global_funcs(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, test_bind::class_wrapper& wrapper, std::string& global_val
)
{
    using asbind20::fp, asbind20::auxiliary, asbind20::aux_value, asbind20::call_conv;

    asbind20::global(engine)
        .function("void set_int(int&out)", fp<&test_bind::set_int>)
        .function(
            "int gen_int()",
            []() -> int
            { return 42; }
        )
        .function(
            "int stdcall_func1(int a, float b)",
            &test_bind::stdcall_func1
        )
        .function(
            "int stdcall_func2(int a, float b)",
            &test_bind::stdcall_func2,
            call_conv<AS_NAMESPACE_QUALIFIER asCALL_STDCALL>
        )
        .function(
            "void set_val(int val)",
            &test_bind::class_wrapper::set_val,
            auxiliary(wrapper)
        )
        .function(
            "int from_aux()",
            &test_bind::from_aux,
            aux_value(1013)
        )
        .property("string val", global_val);
}

static void register_global_funcs(
    asbind20::use_generic_t, AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, test_bind::class_wrapper& wrapper, std::string& global_val
)
{
    using asbind20::fp, asbind20::auxiliary, asbind20::aux_value, asbind20::call_conv;

    asbind20::global<true>(engine)
        .function("void set_int(int&out)", fp<&test_bind::set_int>)
        .function(
            "int gen_int() ",
            []() -> int
            { return 42; }
        )
        .function("void set_val(int val)", fp<&test_bind::class_wrapper::set_val>, auxiliary(wrapper))
        .function(
            "int stdcall_func1(int a, float b)",
            fp<&test_bind::stdcall_func1>
        )
        .function(
            "int stdcall_func2(int a, float b)",
            fp<&test_bind::stdcall_func2>,
            call_conv<AS_NAMESPACE_QUALIFIER asCALL_STDCALL>
        )
        .function(
            "int from_aux()",
            &test_bind::from_aux,
            aux_value(1013)
        )
        .property("string val", global_val);
}

TEST_F(asbind_test_suite, global)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";
    auto* engine = get_engine();

    std::string val = "val";
    test_bind::class_wrapper wrapper{};

    {
        asbind20::global g(engine);
        EXPECT_EQ(g.get_engine(), engine);
        EXPECT_FALSE(g.force_generic());
    }

    register_global_funcs(engine, wrapper, val);

    EXPECT_EQ(val, "val");
    asbind20::ext::exec(engine, "val = \"new string\"");
    EXPECT_EQ(val, "new string");

    EXPECT_EQ(wrapper.value, 0);
    asbind20::ext::exec(engine, "set_val(gen_int())");
    EXPECT_EQ(wrapper.value, 42);
    asbind20::ext::exec(engine, "set_val(from_aux())");
    EXPECT_EQ(wrapper.value, 1013);

    {
        asbind20::request_context ctx(engine);
        auto* stdcall_func1 = engine->GetGlobalFunctionByDecl("int stdcall_func1(int,float)");
        ASSERT_TRUE(stdcall_func1);

        auto result = asbind20::script_invoke<int>(ctx, stdcall_func1, 4, 2.17f);
        EXPECT_TRUE(result_has_value(result));
        EXPECT_EQ(result.value(), 42);
    }

    {
        asbind20::request_context ctx(engine);
        auto* stdcall_func2 = engine->GetGlobalFunctionByDecl("int stdcall_func2(int,float)");
        ASSERT_TRUE(stdcall_func2);

        auto result = asbind20::script_invoke<int>(ctx, stdcall_func2, 4, 2.17f);
        EXPECT_TRUE(result_has_value(result));
        EXPECT_EQ(result.value(), 8);
    }

    {
        asbind20::request_context ctx(engine);
        auto* set_int = engine->GetGlobalFunctionByDecl("void set_int(int&out)");
        ASSERT_TRUE(set_int);

        int out;
        auto result = asbind20::script_invoke<void>(ctx, set_int, std::ref(out));
        EXPECT_TRUE(result_has_value(result));
        EXPECT_EQ(out, 1013);
    }

    {
        asbind20::request_context ctx(engine);
        auto* gen_int = engine->GetGlobalFunctionByDecl("int gen_int()");
        ASSERT_TRUE(gen_int);

        auto result = asbind20::script_invoke<int>(ctx, gen_int);
        EXPECT_EQ(result.value(), 42);
    }
}

TEST_F(asbind_test_suite_generic, global)
{
    auto* engine = get_engine();

    std::string val = "val";
    test_bind::class_wrapper wrapper{};

    {
        asbind20::global<true> g(engine);
        EXPECT_EQ(g.get_engine(), engine);
        EXPECT_TRUE(g.force_generic());
    }

    register_global_funcs(asbind20::use_generic, engine, wrapper, val);

    EXPECT_EQ(val, "val");
    asbind20::ext::exec(engine, "val = \"new string\"");
    EXPECT_EQ(val, "new string");

    EXPECT_EQ(wrapper.value, 0);
    asbind20::ext::exec(engine, "set_val(gen_int())");
    EXPECT_EQ(wrapper.value, 42);
    asbind20::ext::exec(engine, "set_val(from_aux())");
    EXPECT_EQ(wrapper.value, 1013);

    {
        asbind20::request_context ctx(engine);
        auto* stdcall_func1 = engine->GetGlobalFunctionByDecl("int stdcall_func1(int,float)");
        ASSERT_TRUE(stdcall_func1);

        auto result = asbind20::script_invoke<int>(ctx, stdcall_func1, 4, 2.17f);
        EXPECT_TRUE(result_has_value(result));
        EXPECT_EQ(result.value(), 42);
    }

    {
        asbind20::request_context ctx(engine);
        auto* stdcall_func2 = engine->GetGlobalFunctionByDecl("int stdcall_func2(int,float)");
        ASSERT_TRUE(stdcall_func2);

        auto result = asbind20::script_invoke<int>(ctx, stdcall_func2, 4, 2.17f);
        EXPECT_TRUE(result_has_value(result));
        EXPECT_EQ(result.value(), 8);
    }

    {
        asbind20::request_context ctx(engine);
        auto* set_int = engine->GetGlobalFunctionByDecl("void set_int(int&out)");
        ASSERT_TRUE(set_int);

        int out;
        auto result = asbind20::script_invoke<void>(ctx, set_int, std::ref(out));
        EXPECT_TRUE(result_has_value(result));
        EXPECT_EQ(out, 1013);
    }

    {
        asbind20::request_context ctx(engine);
        auto* gen_int = engine->GetGlobalFunctionByDecl("int gen_int()");
        ASSERT_TRUE(gen_int);

        auto result = asbind20::script_invoke<int>(ctx, gen_int);
        EXPECT_EQ(result.value(), 42);
    }
}
