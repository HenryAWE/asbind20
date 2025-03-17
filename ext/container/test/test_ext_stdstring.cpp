#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/stdstring.hpp>
#include <asbind20/ext/assert.hpp>

using namespace asbind_test;

TEST(unicode_support, remove_prefix_and_suffix)
{
    using namespace std::literals;
    using namespace asbind20;

    EXPECT_EQ(ext::utf8::u8_remove_prefix("hello world!", 6), "world!"sv);
    EXPECT_EQ(ext::utf8::u8_remove_suffix("hello world!", 1), "hello world"sv);
}

TEST(unicode_support, substr)
{
    using namespace std::literals;
    using namespace asbind20;

    EXPECT_EQ(ext::utf8::u8_substr("hello world", 7, 2), "or");
    EXPECT_EQ(ext::utf8::u8_substr_r("hello world", 4, 2), "or");

    EXPECT_EQ(
        ext::script_string::string_substr("hello world", 7, 2), "or"
    );
    EXPECT_EQ(
        ext::script_string::string_substr("hello world", -4, 2), "or"
    );
}

TEST(unicode_support, index)
{
    using namespace asbind20;

    EXPECT_EQ(
        ext::script_string::string_opIndex("hello world", 0), U'h'
    );
    EXPECT_EQ(
        ext::script_string::string_opIndex("hello world", -1), U'd'
    );
}

TEST(unicode_support, const_string_iterator)
{
    using asbind20::ext::utf8::string_cbegin;
    using asbind20::ext::utf8::string_cend;

    std::string_view str = "1234";

    std::vector<char32_t> out;
    for(auto it = string_cbegin(str); it != string_cend(str); ++it)
    {
        out.push_back(*it);
    }

    EXPECT_EQ(out, (std::vector<char32_t>{'1', '2', '3', '4'}));
}

namespace test_ext_string
{

std::string get_string_result(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    const char* section,
    std::string_view code
)
{
    int r = 0;

    std::string func_code = asbind20::string_concat(
        "string test_ext_string(){\n",
        code,
        "\n;}"
    );

    auto* m = engine->GetModule(
        "test_ext_string", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    AS_NAMESPACE_QUALIFIER asIScriptFunction* f = nullptr;
    r = m->CompileFunction(
        section,
        func_code.c_str(),
        -1,
        0,
        &f
    );
    EXPECT_GE(r, 0) << "Failed to compile section \"" << section << '\"';

    EXPECT_TRUE(f != nullptr);
    if(!f)
        return std::string();
    asbind20::request_context ctx(engine);
    auto result = asbind20::script_invoke<std::string>(ctx, f);
    f->Release();

    if(result.error() == AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION)
    {
        EXPECT_EQ(result.error(), AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED)
            << "GetExceptionString: " << ctx->GetExceptionString() << '\n'
            << "section: " << ctx->GetExceptionFunction()->GetScriptSectionName();
    }
    else
        EXPECT_TRUE(asbind_test::result_has_value(result));

    return result.value();
}

class ext_string_suite_base : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_engine = asbind20::make_script_engine();
        asbind_test::setup_message_callback(m_engine);
        asbind_test::setup_exception_translator(m_engine);
        asbind20::ext::register_script_assert(
            m_engine,
            [](std::string_view msg)
            {
                auto* ctx = asbind20::current_context();
                if(ctx)
                {
                    FAIL() << "string assertion failed in \"" << ctx->GetFunction()->GetScriptSectionName() << "\": " << msg;
                }
                else
                {
                    FAIL() << "string assertion failed: " << msg;
                }
            }
        );
    }

    auto get_engine() const
        -> AS_NAMESPACE_QUALIFIER asIScriptEngine*
    {
        return m_engine.get();
    }

    void expect_result(
        const std::string& expected_str,
        std::string_view code
    )
    {
        std::string result = get_string_result(
            get_engine(),
            ::testing::UnitTest::GetInstance()->GetInstance()->current_test_info()->test_case_name(),
            code
        );
        EXPECT_EQ(result, expected_str);
    }

private:
    asbind20::script_engine m_engine;
};

template <bool UseGeneric>
class basic_ext_string_test_suite : public ext_string_suite_base
{
public:
    void SetUp() override
    {
        using namespace asbind20;

        if constexpr(!UseGeneric)
        {
            if(has_max_portability())
                GTEST_SKIP() << "AS_MAX_PORTABLITY";
        }

        ext_string_suite_base::SetUp();

        auto* engine = get_engine();

        ext::configure_engine_for_ext_string(engine);
        ext::register_script_char(engine, UseGeneric);
        ext::register_std_string(engine, true, UseGeneric);
        ext::register_string_utils(engine, UseGeneric);
    }
};
} // namespace test_ext_string

using ext_string_generic = test_ext_string::basic_ext_string_test_suite<true>;
using ext_string_native = test_ext_string::basic_ext_string_test_suite<false>;

namespace test_ext_string
{
void test_constructor(ext_string_suite_base& suite)
{
    suite.expect_result(
        "string factory",
        "return \"string factory\""
    );

    suite.expect_result(
        "",
        "return string()"
    );

    suite.expect_result(
        "AAA",
        "return string(3, 'A')"
    );
}
} // namespace test_ext_string

TEST_F(ext_string_native, constructor)
{
    test_constructor(*this);
}

TEST_F(ext_string_generic, constructor)
{
    test_constructor(*this);
}

namespace test_ext_string
{
void test_remove_prefix_and_suffix(ext_string_suite_base& suite)
{
    suite.expect_result(
        "string factory",
        "return \"string factory\".remove_prefix(0)"
    );
    suite.expect_result(
        "string factory",
        "return \"string factory\".remove_suffix(0)"
    );

    suite.expect_result(
        "factory",
        "return \"string factory\".remove_prefix(7)"
    );
    suite.expect_result(
        "string",
        "return \"string factory\".remove_suffix(8)"
    );

    suite.expect_result(
        "",
        "return \"string factory\".remove_prefix(100)"
    );
    suite.expect_result(
        "",
        "return \"string factory\".remove_suffix(100)"
    );
}
} // namespace test_ext_string

TEST_F(ext_string_native, remove_prefix_and_suffix)
{
    test_remove_prefix_and_suffix(*this);
}

TEST_F(ext_string_generic, remove_prefix_and_suffix)
{
    test_remove_prefix_and_suffix(*this);
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
    auto* engine = get_engine();
    auto* m = engine->GetModule("host_script_string_interop", asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "test_script_string.as",
        "string create_str() { return \"hello\"; }\n"
        "void output_ref(string &out str) { str = \"hello\" + \" from ref\"; }\n"
        "void check_str(const string &in str) { assert(str == \"world\"); }\n"
        "void check_str_val(string str) { assert(str == \"world\"); }\n"
        "uint64 get_hash(const string&in str) { return str.hash(); }"
    );
    ASSERT_GE(m->Build(), 0);

    asbind20::request_context ctx(engine);
    {
        auto str = asbind20::script_invoke<std::string>(ctx, m->GetFunctionByName("create_str"));
        ASSERT_TRUE(result_has_value(str));
        EXPECT_EQ(str.value(), "hello");
    }

    {
        std::string str = "origin";
        auto result = asbind20::script_invoke<void>(
            ctx, m->GetFunctionByName("output_ref"), std::ref(str)
        );
        ASSERT_TRUE(result_has_value(result));
        EXPECT_EQ(str, "hello from ref");
    }

    {
        std::string str = "world";
        auto result = asbind20::script_invoke<void>(
            ctx, m->GetFunctionByName("check_str"), std::cref(str)
        );
        ASSERT_TRUE(result_has_value(result));
    }

    {
        std::string str = "world";
        auto result = asbind20::script_invoke<void>(
            ctx, m->GetFunctionByName("check_str"), str
        );
        EXPECT_TRUE(result_has_value(result));
    }

    {
        const std::string str = "hash";
        auto result = asbind20::script_invoke<std::uint64_t>(
            ctx, m->GetFunctionByName("get_hash"), std::cref(str)
        );
        EXPECT_TRUE(result_has_value(result));
        const std::uint64_t expected_val = std::hash<std::string_view>{}(str);
        EXPECT_EQ(result.value(), expected_val);
    }
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
