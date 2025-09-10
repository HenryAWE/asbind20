#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/vocabulary.hpp>
#include <asbind20/ext/stdstring.hpp>
#include <asbind20/ext/array.hpp>

using namespace asbind_test;

TEST(test_invoke, common_types)
{
    using namespace asbind20;
    using asbind_test::result_has_value;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    ext::register_std_string(engine);
    ext::register_script_array(engine);
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
        "array<int>@ test_handle(int v0, int v1) { int[] a = {v0, v1}; return a; }"
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

    {
        auto* fp = m->GetFunctionByName("test_handle");
        ASSERT_NE(fp, nullptr);

        asbind20::request_context ctx(engine);

        auto result = asbind20::script_invoke<ext::script_array*>(ctx, fp, 10, 13);
        ASSERT_TRUE(result_has_value(result));
        EXPECT_EQ(result->size(), 2);
        EXPECT_EQ(*(int*)result->opIndex(0), 10);
        EXPECT_EQ(*(int*)result->opIndex(1), 13);
    }

    {
        auto* fp = m->GetFunctionByName("test_handle");
        ASSERT_NE(fp, nullptr);

        asbind20::request_context ctx(engine);

        // Ignore returned handle by void type
        auto result = asbind20::script_invoke<void>(ctx, fp, 10, 13);
        ASSERT_TRUE(result_has_value(result));

        // This should not be complained by ASan
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

namespace test_invoke
{
template <typename T>
static ::testing::AssertionResult check_result_ex(
    const asbind20::script_invoke_result<T>& r,
    AS_NAMESPACE_QUALIFIER asEContextState expected_thrown_ec = AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION
)
{
    try
    {
        (void)r.value();
        return testing::AssertionFailure() << "Exception is not thrown";
    }
    catch(const asbind20::bad_script_invoke_result_access& e)
    {
        EXPECT_STREQ(e.what(), "bad script invoke result access");

        using asbind20::to_string;

        if(auto actual = e.error(); actual != expected_thrown_ec)
        {
            return testing::AssertionFailure()
                   << "Unexpected asEContextState error code\n"
                   << "expected: " << to_string(expected_thrown_ec) << '\n'
                   << "actual: " << to_string(actual);
        }

        return testing::AssertionSuccess();
    }
}
} // namespace test_invoke

TEST(test_invoke, bad_result)
{
    using namespace asbind20;
    using asbind_test::result_has_value;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);
    asbind_test::setup_exception_translator(engine);

    global<true>(engine)
        .function(
            "void throw_err()",
            []() -> void
            { throw std::runtime_error("throw_err"); }
        );

    auto* m = engine->GetModule(
        "test_custom_rule", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    m->AddScriptSection(
        "bad_result",
        "int test0() { throw_err(); return 42; }\n"
        "int placeholder = 42;\n"
        "int& test1() { throw_err(); return placeholder; }\n"
        "void test2() { throw_err(); }"
    );
    ASSERT_GE(m->Build(), 0);

    {
        auto* f = m->GetFunctionByName("test0");
        ASSERT_TRUE(f);

        request_context ctx(engine);
        auto result = script_invoke<int>(ctx, f);

        ASSERT_FALSE(result_has_value(result));
        EXPECT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION);

        EXPECT_THROW((void)result.value(), bad_script_invoke_result_access);
        EXPECT_TRUE(test_invoke::check_result_ex(result));
    }

    {
        auto* f = m->GetFunctionByName("test0");
        ASSERT_TRUE(f);

        request_context ctx(engine);
        // Ignore the int result by setting the template argument to void
        auto result = script_invoke<void>(ctx, f);

        ASSERT_FALSE(result_has_value(result));
        EXPECT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION);

        EXPECT_THROW((void)result.value(), bad_script_invoke_result_access);
    }

    {
        auto* f = m->GetFunctionByName("test1");
        ASSERT_TRUE(f);

        request_context ctx(engine);
        auto result = script_invoke<int&>(ctx, f);

        ASSERT_FALSE(result_has_value(result));
        EXPECT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION);

        EXPECT_THROW((void)result.value(), bad_script_invoke_result_access);
        EXPECT_TRUE(test_invoke::check_result_ex(result));
    }

    {
        auto* f = m->GetFunctionByName("test2");
        ASSERT_TRUE(f);

        request_context ctx(engine);
        auto result = script_invoke<void>(ctx, f);

        ASSERT_FALSE(result_has_value(result));
        EXPECT_FALSE(result.has_value());
        EXPECT_EQ(result.error(), AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION);

        EXPECT_THROW((void)result.value(), bad_script_invoke_result_access);
        EXPECT_TRUE(test_invoke::check_result_ex(result));
    }
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
