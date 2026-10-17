#include <asbind_test/framework.hpp>
#include "test_script_string.hpp"
#include <asbind20/io/section.hpp>
#include <asbind20/io/to_string.hpp>

namespace test_script_string
{
template <bool UseGeneric>
class test_string_factory : public script_string_suite_base<UseGeneric>
{
public:
    [[nodiscard]]
    std::string run_string(std::string_view fragment) const
    {
        auto* engine = this->get_engine();
        auto* tmp_module = asbind20::create_module(
            engine, "test_string_factory"
        );

        std::string code = asbind20::string_concat(
            "string func(){\n",
            fragment,
            "\n}"
        );

        asbind20::io::load_string(
            tmp_module,
            "code",
            code,
            -1
        );
        if(tmp_module->Build() < 0)
        {
            ADD_FAILURE() << "Failed to build module";
            return {};
        }

        auto f = tmp_module->GetFunctionByDecl("string func()");
        if(!f)
        {
            ADD_FAILURE() << "No function";
            return std::string();
        }

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<std::string>(ctx, f);
        if(!result.has_value())
        {
            using asbind20::to_string;
            ADD_FAILURE() << "No result: " << to_string(result.error());
            return std::string();
        }

        return result.value();
    }
};
} // namespace test_script_string

using TestStringFactoryGeneric = test_script_string::test_string_factory<true>;
using TestStringFactoryNative = test_script_string::test_string_factory<false>;

TEST_F(TestStringFactoryGeneric, RunScript)
{
    auto result = this->run_string(
        R"(return "test";)"
    );
    EXPECT_EQ(result, "test");
}

TEST_F(TestStringFactoryNative, RunScript)
{
    auto result = this->run_string(
        R"(return "test";)"
    );
    EXPECT_EQ(result, "test");
}

TEST_F(TestStringFactoryGeneric, Add)
{
    auto result = this->run_string(
        R"(return "test" + " add";)"
    );
    EXPECT_EQ(result, "test add");
}

TEST_F(TestStringFactoryNative, Add)
{
    auto result = this->run_string(
        R"(return "test" + " add";)"
    );
    EXPECT_EQ(result, "test add");
}

TEST_F(TestStringFactoryGeneric, Compare)
{
    {
        auto result = this->run_string(
            R"(return "aaa" < "bbb" ? "true" : "false";)"
        );
        EXPECT_EQ(result, "true");
    }

    {
        auto result = this->run_string(
            R"(return "bbb" < "aaa" ? "true" : "false";)"
        );
        EXPECT_EQ(result, "false");
    }
}

TEST_F(TestStringFactoryNative, Compare)
{
    {
        auto result = this->run_string(
            R"(return "aaa" < "bbb" ? "true" : "false";)"
        );
        EXPECT_EQ(result, "true");
    }

    {
        auto result = this->run_string(
            R"(return "bbb" < "aaa" ? "true" : "false";)"
        );
        EXPECT_EQ(result, "false");
    }
}

using ScriptStringGeneric = test_script_string::script_string_suite_base<true>;
using ScriptStringNative = test_script_string::script_string_suite_base<false>;

namespace test_script_string
{
static void test_ctor_fn(asbind20::engine_pointer engine)
{
    auto* m = asbind20::create_module(engine, "test");
    m->AddScriptSection(
        "test",
        "string f(uint count) { return string(count, 'A'); }"
    );
    ASSERT_GE(m->Build(), 0);
    auto* f = m->GetFunctionByName("f");
    ASSERT_THAT(f, ::testing::NotNull());

    {
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<std::string>(ctx, f, 3U);
        ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
        EXPECT_EQ(result.value(), "AAA");
    }

    {
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<std::string>(ctx, f, 5U);
        ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
        EXPECT_EQ(result.value(), "AAAAA");
    }
}
} // namespace test_script_string

TEST_F(ScriptStringGeneric, CtorFn)
{
    auto engine = get_engine();
    test_script_string::test_ctor_fn(engine);
}

TEST_F(ScriptStringNative, CtorFn)
{
    auto engine = get_engine();
    test_script_string::test_ctor_fn(engine);
}

TEST_F(TestStringFactoryGeneric, Constructor)
{
    EXPECT_EQ(this->run_string(R"(return "string factory";)"), "string factory");
    EXPECT_EQ(this->run_string(R"(return string();)"), "");
    EXPECT_EQ(this->run_string(R"(return string(3, 'A');)"), "AAA");
}

TEST_F(TestStringFactoryNative, Constructor)
{
    EXPECT_EQ(this->run_string(R"(return "string factory";)"), "string factory");
    EXPECT_EQ(this->run_string(R"(return string();)"), "");
    EXPECT_EQ(this->run_string(R"(return string(3, 'A');)"), "AAA");
}

TEST_F(TestStringFactoryGeneric, RemovePrefixAndSuffix)
{
    EXPECT_EQ(
        this->run_string(R"(return "string factory".remove_prefix(0);)"),
        "string factory"
    );
    EXPECT_EQ(
        this->run_string(R"(return "string factory".remove_suffix(0);)"),
        "string factory"
    );
    EXPECT_EQ(
        this->run_string(R"(return "string factory".remove_prefix(7);)"),
        "factory"
    );
    EXPECT_EQ(
        this->run_string(R"(return "string factory".remove_suffix(8);)"),
        "string"
    );
    EXPECT_EQ(
        this->run_string(R"(return "string factory".remove_prefix(100);)"),
        ""
    );
    EXPECT_EQ(
        this->run_string(R"(return "string factory".remove_suffix(100);)"),
        ""
    );
}

TEST_F(TestStringFactoryNative, RemovePrefixAndSuffix)
{
    EXPECT_EQ(
        this->run_string(R"(return "string factory".remove_prefix(0);)"),
        "string factory"
    );
    EXPECT_EQ(
        this->run_string(R"(return "string factory".remove_suffix(0);)"),
        "string factory"
    );
    EXPECT_EQ(
        this->run_string(R"(return "string factory".remove_prefix(7);)"),
        "factory"
    );
    EXPECT_EQ(
        this->run_string(R"(return "string factory".remove_suffix(8);)"),
        "string"
    );
    EXPECT_EQ(
        this->run_string(R"(return "string factory".remove_prefix(100);)"),
        ""
    );
    EXPECT_EQ(
        this->run_string(R"(return "string factory".remove_suffix(100);)"),
        ""
    );
}

namespace test_script_string
{
static void test_host_interop(asbind20::engine_pointer engine)
{
    auto* m = asbind20::create_module(engine, "host_script_string_interop");
    ASSERT_THAT(m, ::testing::NotNull());

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
        auto str = asbind20::script_invoke<std::string>(
            ctx, m->GetFunctionByName("create_str")
        );
        ASBIND_TEST_ASSERT_INVOKE_RESULT(str);
        EXPECT_EQ(str.value(), "hello");
    }

    {
        std::string str = "origin";
        auto result = asbind20::script_invoke<void>(
            ctx, m->GetFunctionByName("output_ref"), std::ref(str)
        );
        ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
        EXPECT_EQ(str, "hello from ref");
    }

    {
        std::string str = "world";
        auto result = asbind20::script_invoke<void>(
            ctx, m->GetFunctionByName("check_str"), std::cref(str)
        );
        ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
    }

    {
        std::string str = "world";
        auto result = asbind20::script_invoke<void>(
            ctx, m->GetFunctionByName("check_str"), str
        );
        ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
    }

    {
        const std::string str = "hash";
        auto result = asbind20::script_invoke<std::uint64_t>(
            ctx, m->GetFunctionByName("get_hash"), std::cref(str)
        );
        ASBIND_TEST_ASSERT_INVOKE_RESULT(result);
        const std::uint64_t expected_val = std::hash<std::string_view>{}(str);
        EXPECT_EQ(result.value(), expected_val);
    }
}
} // namespace test_script_string

TEST_F(ScriptStringGeneric, HostInterop)
{
    asbind_test::setup_script_assertion(get_engine());
    test_script_string::test_host_interop(get_engine());
}

TEST_F(ScriptStringNative, HostInterop)
{
    asbind_test::setup_script_assertion(get_engine());
    test_script_string::test_host_interop(get_engine());
}
