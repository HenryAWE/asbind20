#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>

using namespace asbind_test;

struct class_wrapper
{
    int value = 0;

    void set_val(int val)
    {
        value = val;
    }
};

static void register_global_funcs(
    asIScriptEngine* engine, class_wrapper& wrapper, std::string& global_val
)
{
    asbind20::global(engine)
        .function(
            "int gen_int()",
            +[]() -> int
            { return 42; }
        )
        .function(
            "void set_val(int val)",
            &class_wrapper::set_val,
            wrapper
        )
        .property("string val", global_val);
}

static void register_global_funcs(
    asbind20::use_generic_t, asIScriptEngine* engine, class_wrapper& wrapper, std::string& global_val
)
{
    asbind20::global<true>(engine)
        .function(
            "int gen_int() ",
            +[](asIScriptGeneric* gen) -> void
            {
                asbind20::set_generic_return<int>(gen, 42);
            }
        )
        .function<&class_wrapper::set_val>("void set_val(int val)", wrapper)
        .property("string val", global_val);
}

TEST_F(asbind_test_suite, global)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";
    asIScriptEngine* engine = get_engine();

    std::string val = "val";
    class_wrapper wrapper{};

    register_global_funcs(engine, wrapper, val);

    EXPECT_EQ(val, "val");
    asbind20::exec(engine, "val = \"new string\"");
    EXPECT_EQ(val, "new string");

    EXPECT_EQ(wrapper.value, 0);
    asbind20::exec(engine, "set_val(gen_int())");
    EXPECT_EQ(wrapper.value, 42);

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
    class_wrapper wrapper{};

    register_global_funcs(asbind20::use_generic, engine, wrapper, val);

    EXPECT_EQ(val, "val");
    asbind20::exec(engine, "val = \"new string\"");
    EXPECT_EQ(val, "new string");

    EXPECT_EQ(wrapper.value, 0);
    asbind20::exec(engine, "set_val(gen_int())");
    EXPECT_EQ(wrapper.value, 42);

    {
        asbind20::request_context ctx(engine);
        asIScriptFunction* gen_int = engine->GetGlobalFunctionByDecl("int gen_int()");
        ASSERT_TRUE(gen_int);

        auto result = asbind20::script_invoke<int>(ctx, gen_int);
        EXPECT_EQ(result.value(), 42);
    }
}
