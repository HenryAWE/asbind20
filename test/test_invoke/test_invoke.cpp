#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/vocabulary.hpp>
#include <asbind20/ext/stdstring.hpp>

using namespace asbind_test;

TEST(test_invoke, common_types)
{
    using namespace asbind20;
    using asbind_test::result_has_value;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    ext::register_std_string(engine);
    auto* m = engine->GetModule(
        "test_invoke", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );

    m->AddScriptSection(
        "test_invoke.as",
        "int add_1(int i) { return i + 1; }\n"
        "void add_ref_1(int i, int& out o) { o = i + 1; }\n"
        "float flt_identity(float val) { return val; }\n"
        "double dbl_identity(double val) { return val; }\n"
        "string test(int a, int&out b) { b = a + 1; return \"test\"; }"
    );
    ASSERT_GE(m->Build(), 0);

    {
        auto* fp = m->GetFunctionByName("add_1");
        ASSERT_NE(fp, nullptr);

        asbind20::request_context ctx(engine);

        EXPECT_EQ(asbind20::script_invoke<int>(ctx, fp, 0).value(), 1);
        EXPECT_EQ(asbind20::script_invoke<int>(ctx, fp, 1).value(), 2);
    }

    {
        auto* fp = m->GetFunctionByName("add_ref_1");
        ASSERT_NE(fp, nullptr);

        asbind20::request_context ctx(engine);

        int val = 0;
        asbind20::script_invoke<void>(ctx, fp, 1, std::ref(val));
        EXPECT_EQ(val, 2);
    }

    {
        auto* fp = m->GetFunctionByName("flt_identity");
        ASSERT_NE(fp, nullptr);

        asbind20::request_context ctx(engine);

        auto result = asbind20::script_invoke<float>(ctx, fp, 3.14f);
        ASSERT_TRUE(result_has_value(result));
        EXPECT_FLOAT_EQ(result.value(), 3.14f);
    }

    {
        auto* fp = m->GetFunctionByName("dbl_identity");
        ASSERT_NE(fp, nullptr);

        asbind20::request_context ctx(engine);

        auto result = asbind20::script_invoke<double>(ctx, fp, 3.14);
        ASSERT_TRUE(result_has_value(result));
        EXPECT_DOUBLE_EQ(result.value(), 3.14);
    }

    {
        auto* fp = m->GetFunctionByName("test");
        ASSERT_NE(fp, nullptr);

        asbind20::request_context ctx(engine);

        int val = 0;
        auto result = asbind20::script_invoke<std::string>(ctx, fp, 1, std::ref(val));
        ASSERT_TRUE(result_has_value(result));
        EXPECT_EQ(result.value(), "test");
        EXPECT_EQ(val, 2);
    }
}

TEST(test_invoke, custom_rule_byte)
{
    using namespace asbind20;
    using asbind_test::result_has_value;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    auto* m = engine->GetModule(
        "test_custom_rule", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );

    m->AddScriptSection(
        "test_custom_rule.as",
        "uint8 add_1(uint8 i) { return i + 1; }"
    );
    ASSERT_GE(m->Build(), 0);

    {
        auto* add_1 = m->GetFunctionByName("add_1");

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<std::byte>(ctx, add_1, std::byte(0x1));

        ASSERT_TRUE(result_has_value(result));
        EXPECT_EQ(result.value(), std::byte(0x2));
    }
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
