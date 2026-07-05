#include <asbind_test/framework.hpp>
#include "test_script_string.hpp"
#include <asbind20/io/to_string.hpp>

namespace test_script_string
{
template <bool UseGeneric>
class test_string_factory : public script_string_suite_base<UseGeneric>
{
public:
    std::string run_string(std::string_view fragment) const
    {
        auto* engine = this->get_engine();
        AS_NAMESPACE_QUALIFIER asIScriptModule* tmp_module = engine->GetModule(
            "test_string_factory", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
        );

        std::string code;
        code += "string func(){\n";
        code += fragment;
        code += "\n}";

        tmp_module->AddScriptSection(
            "code", code.c_str(), code.size(), -1
        );
        if(tmp_module->Build() < 0)
        {
            ADD_FAILURE() << "Failed to build module";
            return std::string();
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
    auto* m = engine->GetModule(
        "test", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    m->AddScriptSection(
        "test",
        "string f(uint count) { return string(count, 'A'); }"
    );
    ASSERT_GE(m->Build(), 0);
    auto* f = m->GetFunctionByName("f");
    ASSERT_NE(f, nullptr);

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
