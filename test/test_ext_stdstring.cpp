#include <gtest/gtest.h>
#include "shared.hpp"
#include <asbind20/asbind.hpp>
#include <asbind20/ext/stdstring.hpp>

using namespace asbind_test;

TEST(ext_string, unicode_support)
{
    using namespace std::literals;
    using namespace asbind20;

    EXPECT_EQ(ext::u8_remove_prefix("hello world!", 6), "world!"sv);
    EXPECT_EQ(ext::u8_remove_suffix("hello world!", 1), "hello world"sv);

    EXPECT_EQ(ext::u8_substr("hello world", 7, 2), "or");
}

TEST_F(asbind_test_suite, ext_stdstring)
{
    run_file("script/test_string.as");
}

TEST_F(asbind_test_suite, host_script_string_interop)
{
    asIScriptEngine* engine = get_engine();
    asIScriptModule* m = engine->GetModule("host_script_string_interop", asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "test_script_string.as",
        "string create_str() { return \"hello\"; }"
        "void output_ref(string &out str) { str = \"hello\" + \" from ref\"; }"
        "void check_str(string &in str) { assert(str == \"world\"); }"
    );
    ASSERT_GE(m->Build(), 0);

    asIScriptContext* ctx = engine->CreateContext();
    {
        auto str = asbind20::script_invoke<std::string>(ctx, m->GetFunctionByName("create_str"));
        ASSERT_TRUE(result_has_value(str));
        EXPECT_EQ(str.value(), "hello");
    }

    {
        std::string str = "origin";
        auto result = asbind20::script_invoke<void>(ctx, m->GetFunctionByName("output_ref"), std::ref(str));
        ASSERT_TRUE(result_has_value(result));
        EXPECT_EQ(str, "hello from ref");
    }

    {
        std::string str = "world";
        auto result = asbind20::script_invoke<void>(ctx, m->GetFunctionByName("check_str"), str);
        ASSERT_TRUE(result_has_value(result));
    }

    ctx->Release();
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
