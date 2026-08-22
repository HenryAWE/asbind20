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
        "float f(int arg = 3) { return 0.0; }"
    );
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByName("f");
    ASSERT_THAT(f, ::testing::NotNull());

    {
        auto param_info = asbind20::get_func_param_info(f, 0);
        EXPECT_EQ(param_info.name, "arg");
        EXPECT_EQ(param_info.type_id, AS_NAMESPACE_QUALIFIER asTYPEID_INT32);
        EXPECT_EQ(param_info.flags, 0);
        EXPECT_EQ(param_info.default_arg, "3");
    }

    {
        int var_idx = m->GetGlobalVarIndexByName("var");
        ASSERT_GE(var_idx, 0);

        auto var_info = asbind20::get_global_var_info(m, var_idx);
        EXPECT_EQ(var_info.name, "var");
        EXPECT_EQ(var_info.type_id, AS_NAMESPACE_QUALIFIER asTYPEID_INT32);
        EXPECT_FALSE(var_info.is_const);
    }
}
