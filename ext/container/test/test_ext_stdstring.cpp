#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/stdstring.hpp>

using namespace asbind_test;

TEST(unicode_support, remove_prefix_and_suffix)
{
    using namespace std::literals;
    using namespace asbind20;

    EXPECT_EQ(ext::u8_remove_prefix("hello world!", 6), "world!"sv);
    EXPECT_EQ(ext::u8_remove_suffix("hello world!", 1), "hello world"sv);
}

TEST(unicode_support, substr)
{
    using namespace std::literals;
    using namespace asbind20;

    EXPECT_EQ(ext::u8_substr("hello world", 7, 2), "or");
}

TEST(unicode_support, const_string_iterator)
{
    using asbind20::ext::string_cbegin;
    using asbind20::ext::string_cend;

    std::string_view str = "1234";

    std::vector<char32_t> out;
    for(auto it = string_cbegin(str); it != string_cend(str); ++it)
    {
        out.push_back(*it);
    }

    EXPECT_EQ(out, (std::vector<char32_t>{'1', '2', '3', '4'}));
}

TEST_F(asbind_test_suite, ext_stdstring)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    run_file("script/test_string.as");
}

TEST_F(asbind_test_suite_generic, ext_stdstring)
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
