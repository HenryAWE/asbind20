#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/exec.hpp>

using namespace asbind_test;

namespace test_bind
{
void set_int(int& out)
{
    out = 1013;
}

struct class_wrapper
{
    int value = 0;

    void set_val(int val)
    {
        value = val;
    }
};
} // namespace test_bind

static void register_global_funcs(
    asIScriptEngine* engine, test_bind::class_wrapper& wrapper, std::string& global_val
)
{
    using asbind20::fp, asbind20::auxiliary;

    asbind20::global(engine)
        .function("void set_int(int&out)", fp<&test_bind::set_int>)
        .function(
            "int gen_int()",
            []() -> int
            { return 42; }
        )
        .function(
            "void set_val(int val)",
            &test_bind::class_wrapper::set_val,
            auxiliary(wrapper)
        )
        .property("string val", global_val);
}

static void register_global_funcs(
    asbind20::use_generic_t, asIScriptEngine* engine, test_bind::class_wrapper& wrapper, std::string& global_val
)
{
    using asbind20::fp, asbind20::auxiliary;

    asbind20::global<true>(engine)
        .function("void set_int(int&out)", fp<&test_bind::set_int>)
        .function(
            "int gen_int() ",
            []() -> int
            { return 42; }
        )
        .function("void set_val(int val)", fp<&test_bind::class_wrapper::set_val>, auxiliary(wrapper))
        .property("string val", global_val);
}

TEST_F(asbind_test_suite, global)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";
    asIScriptEngine* engine = get_engine();

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

    {
        asbind20::request_context ctx(engine);
        asIScriptFunction* set_int = engine->GetGlobalFunctionByDecl("void set_int(int&out)");
        ASSERT_TRUE(set_int);

        int out;
        auto result = asbind20::script_invoke<void>(ctx, set_int, std::ref(out));
        EXPECT_TRUE(result_has_value(result));
        EXPECT_EQ(out, 1013);
    }

    {
        asbind20::request_context ctx(engine);
        asIScriptFunction* gen_int = engine->GetGlobalFunctionByDecl("int gen_int()");
        ASSERT_TRUE(gen_int);

        auto result = asbind20::script_invoke<int>(ctx, gen_int);
        EXPECT_EQ(result.value(), 42);
    }
}

TEST_F(asbind_test_suite_generic, global)
{
    asIScriptEngine* engine = get_engine();

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

    {
        asbind20::request_context ctx(engine);
        asIScriptFunction* set_int = engine->GetGlobalFunctionByDecl("void set_int(int&out)");
        ASSERT_TRUE(set_int);

        int out;
        auto result = asbind20::script_invoke<void>(ctx, set_int, std::ref(out));
        EXPECT_TRUE(result_has_value(result));
        EXPECT_EQ(out, 1013);
    }

    {
        asbind20::request_context ctx(engine);
        asIScriptFunction* gen_int = engine->GetGlobalFunctionByDecl("int gen_int()");
        ASSERT_TRUE(gen_int);

        auto result = asbind20::script_invoke<int>(ctx, gen_int);
        EXPECT_EQ(result.value(), 42);
    }
}
