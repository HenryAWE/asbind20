#include <asbind_test/framework.hpp>
#include <asbind20/asbind.hpp>
#include <asbind20/util/script_refl.hpp>

TEST(ScriptRefl, ScriptModule)
{
    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine);
    auto* m = asbind20::create_module(
        engine, "test_refl"
    );
    m->AddScriptSection(
        "test_refl",
        "int var = 0;\n"
        "float f(float, int arg = 3) { return 0.0; }\n"
        "int g(int a) { int local_var = a; return local_var; }\n"
    );
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByName("f");
    ASSERT_THAT(f, ::testing::NotNull());

    {
        auto param_info = asbind20::get_func_param_info(f, 0);
        ASSERT_TRUE(param_info);
        EXPECT_THAT(param_info->name, ::testing::IsEmpty());
        EXPECT_EQ(param_info->type_id, AS_NAMESPACE_QUALIFIER asTYPEID_FLOAT);
        EXPECT_EQ(param_info->flags, 0);
        EXPECT_THAT(
            param_info->default_arg,
            ::testing::IsEmpty()
        );
    }

    {
        auto param_info = asbind20::get_func_param_info(f, 1);
        ASSERT_TRUE(param_info);
        EXPECT_EQ(param_info->name, "arg");
        EXPECT_EQ(param_info->type_id, AS_NAMESPACE_QUALIFIER asTYPEID_INT32);
        EXPECT_EQ(param_info->flags, 0);
        EXPECT_THAT(
            param_info->default_arg,
            ::testing::HasSubstr("3")
        );
    }

    {
        auto* g = m->GetFunctionByName("g");
        ASSERT_THAT(g, ::testing::NotNull());

        // The parameters are stored in the variable list as well
        auto param_var_info = asbind20::get_func_var_info(g, 0);
        ASSERT_TRUE(param_var_info);
        EXPECT_EQ(param_var_info->name, "a");
        EXPECT_EQ(param_var_info->type_id, AS_NAMESPACE_QUALIFIER asTYPEID_INT32);

        auto var_info = asbind20::get_func_var_info(*g, 1);
        ASSERT_TRUE(var_info);
        EXPECT_EQ(var_info->name, "local_var");
        EXPECT_EQ(var_info->type_id, AS_NAMESPACE_QUALIFIER asTYPEID_INT32);
    }

    {
        int var_idx = m->GetGlobalVarIndexByName("var");
        ASSERT_GE(var_idx, 0);

        auto var_info = asbind20::get_global_var_info(m, var_idx);
        ASSERT_TRUE(var_info);
        EXPECT_EQ(var_info->name, "var");
        EXPECT_EQ(var_info->type_id, AS_NAMESPACE_QUALIFIER asTYPEID_INT32);
        EXPECT_FALSE(var_info->is_const);
    }
}

TEST(ScriptRefl, ErrorHandling)
{
    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine);
    auto* m = asbind20::create_module(
        engine, "test_refl_error"
    );
    m->AddScriptSection(
        "test_refl_error",
        "int var = 0;\n"
        "float f(float, int arg = 3) { return 0.0; }\n"
    );
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByName("f");
    ASSERT_THAT(f, ::testing::NotNull());

    // Out-of-range parameter index
    {
        auto param_info = asbind20::get_func_param_info(f, 2);
        EXPECT_FALSE(param_info);
        EXPECT_EQ(param_info.error(), AS_NAMESPACE_QUALIFIER asINVALID_ARG);
    }

    // Null function pointer, parameter info
    {
        auto param_info = asbind20::get_func_param_info(nullptr, 0);
        EXPECT_FALSE(param_info);
        EXPECT_EQ(param_info.error(), AS_NAMESPACE_QUALIFIER asINVALID_ARG);
    }

    // Out-of-range local variable index
    {
        auto var_info = asbind20::get_func_var_info(*f, f->GetVarCount());
        EXPECT_FALSE(var_info);
        EXPECT_EQ(var_info.error(), AS_NAMESPACE_QUALIFIER asINVALID_ARG);
    }

    // Null function pointer, variable info
    {
        auto var_info = asbind20::get_func_var_info(nullptr, 0);
        EXPECT_FALSE(var_info);
        EXPECT_EQ(var_info.error(), AS_NAMESPACE_QUALIFIER asINVALID_ARG);
    }

    // Out-of-range global variable index
    {
        auto var_info = asbind20::get_global_var_info(m, m->GetGlobalVarCount());
        EXPECT_FALSE(var_info);
        EXPECT_EQ(var_info.error(), AS_NAMESPACE_QUALIFIER asINVALID_ARG);
    }

    // Null module pointer
    {
        auto var_info = asbind20::get_global_var_info(nullptr, 0);
        EXPECT_FALSE(var_info);
        EXPECT_EQ(var_info.error(), AS_NAMESPACE_QUALIFIER asINVALID_ARG);
    }
}
