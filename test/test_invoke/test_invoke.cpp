#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/vocabulary.hpp>
#include <asbind20/ext/stdstring.hpp>

using namespace asbind_test;

TEST(script_invoke_result, common)
{
    using namespace asbind20;

    {
        script_invoke_result<int> result(1);

        ASSERT_TRUE(result.has_value());
        ASSERT_TRUE(result);
        EXPECT_EQ(result.value(), 1);
        EXPECT_EQ(result.error(), AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED);
    }

    {
        script_invoke_result<std::string> result(std::string("hello"));

        ASSERT_TRUE(result.has_value());
        ASSERT_TRUE(result);
        EXPECT_EQ(result.value(), "hello");
        EXPECT_EQ(result.error(), AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED);
    }

    {
        script_invoke_result<int> result(bad_result, AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION);

        ASSERT_FALSE(result.has_value());
        ASSERT_FALSE(result);
        EXPECT_THROW((void)result.value(), bad_script_invoke_result_access);
        EXPECT_EQ(result.error(), AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION);
    }
}

TEST(script_invoke_result, reference)
{
    using namespace asbind20;
    {
        int val = 1;

        script_invoke_result<int&> result(val);
        ASSERT_TRUE(result.has_value());
        ASSERT_TRUE(result);
        EXPECT_EQ(result.value(), 1);
        EXPECT_EQ(&result.value(), &val);
        EXPECT_EQ(result.error(), AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED);
    }

    {
        script_invoke_result<int&> result(bad_result, AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION);

        ASSERT_FALSE(result.has_value());
        ASSERT_FALSE(result);
        EXPECT_THROW((void)result.value(), bad_script_invoke_result_access);
        EXPECT_EQ(result.error(), AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION);
    }
}

TEST(script_invoke_result, void)
{
    using namespace asbind20;

    {
        script_invoke_result<void> result;

        ASSERT_TRUE(result.has_value());
        ASSERT_TRUE(result);
        EXPECT_NO_THROW(result.value());
        EXPECT_NO_FATAL_FAILURE((void)*result);
        EXPECT_EQ(result.error(), AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED);
    }

    {
        script_invoke_result<void> result(bad_result, AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION);

        ASSERT_FALSE(result.has_value());
        ASSERT_FALSE(result);
        EXPECT_THROW(result.value(), bad_script_invoke_result_access);
        EXPECT_EQ(result.error(), AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION);
    }
}

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
