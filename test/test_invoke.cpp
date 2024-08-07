#include <gtest/gtest.h>
#include "shared.hpp"
#include <asbind20/asbind.hpp>

TEST(script_invoke_result, common)
{
    using namespace asbind20;

    {
        script_invoke_result<int> result = 1;

        ASSERT_TRUE(result.has_value());
        ASSERT_TRUE(result);
        EXPECT_EQ(result.value(), 1);
        EXPECT_EQ(result.error(), asEXECUTION_FINISHED);
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
        EXPECT_EQ(result.error(), asEXECUTION_FINISHED);
    }
}

template <typename T>
::testing::AssertionResult result_has_value(const asbind20::script_invoke_result<T>& r)
{
    if(r.has_value())
        return ::testing::AssertionSuccess();
    else
    {
        const char* state_str = "";
        switch(r.error())
        {
        case asEXECUTION_FINISHED: state_str = "FINISHED"; break;
        case asEXECUTION_SUSPENDED: state_str = "SUSPENDED"; break;
        case asEXECUTION_ABORTED: state_str = "ABORTED"; break;
        case asEXECUTION_EXCEPTION: state_str = "EXCEPTION"; break;
        case asEXECUTION_PREPARED: state_str = "PREPARED"; break;
        case asEXECUTION_UNINITIALIZED: state_str = "UNINITIALIZED"; break;
        case asEXECUTION_ACTIVE: state_str = "ACTIVE"; break;
        case asEXECUTION_ERROR: state_str = "ERROR"; break;
        case asEXECUTION_DESERIALIZATION: state_str = "DESERIALIZATION"; break;
        }

        return ::testing::AssertionFailure()
               << "r = " << r.error() << ' ' << state_str;
    }
}

using asbind_test::asbind_test_suite;

TEST_F(asbind_test_suite, invoke)
{
    asIScriptEngine* engine = get_engine();
    asIScriptModule* m = engine->GetModule("test_invoke", asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "test_invoke.as",
        "int add_1(int i) { return i + 1; }\n"
        "void add_ref_1(int i, int& out o) { o = i + 1; }\n"
    );
    ASSERT_GE(m->Build(), 0);

    {
        asIScriptFunction* fp = m->GetFunctionByName("add_1");
        ASSERT_NE(fp, nullptr);

        asIScriptContext* ctx = engine->CreateContext();

        EXPECT_EQ(asbind20::script_invoke<int>(ctx, fp, 0).value(), 1);
        EXPECT_EQ(asbind20::script_invoke<int>(ctx, fp, 1).value(), 2);

        ctx->Release();
    }

    {
        asIScriptFunction* fp = m->GetFunctionByName("add_ref_1");
        ASSERT_NE(fp, nullptr);

        asIScriptContext* ctx = engine->CreateContext();

        int val = 0;
        asbind20::script_invoke<void>(ctx, fp, 1, std::ref(val));
        EXPECT_EQ(val, 2);

        ctx->Release();
    }
}

TEST_F(asbind_test_suite, script_class)
{
    asIScriptEngine* engine = get_engine();
    asIScriptModule* m = engine->GetModule("test_script_class", asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "test_invoke.as",
        "class my_class"
        "{"
        "int m_val;"
        "void set_val(int new_val) { m_val = new_val; }"
        "int get_val() const { return m_val; }"
        "int& get_val_ref() { return m_val; }"
        "};"
    );
    ASSERT_GE(m->Build(), 0);

    asITypeInfo* my_class_t = m->GetTypeInfoByName("my_class");
    ASSERT_NE(my_class_t, nullptr);

    asIScriptContext* ctx = engine->CreateContext();
    {
        auto my_class = asbind20::instantiate_class(ctx, my_class_t);

        asIScriptFunction* set_val = my_class_t->GetMethodByDecl("void set_val(int)");
        asbind20::script_invoke<void>(ctx, my_class, set_val, 182375);

        asIScriptFunction* get_val = my_class_t->GetMethodByDecl("int get_val() const");
        auto val = asbind20::script_invoke<int>(ctx, my_class, get_val);

        ASSERT_TRUE(result_has_value(val));
        EXPECT_EQ(val.value(), 182375);

        asIScriptFunction* get_val_ref = my_class_t->GetMethodByDecl("int& get_val_ref()");
        auto val_ref = asbind20::script_invoke<int&>(ctx, my_class, get_val_ref);

        ASSERT_TRUE(result_has_value(val_ref));
        EXPECT_EQ(val_ref.value(), 182375);

        *val_ref = 182376;

        val = asbind20::script_invoke<int>(ctx, my_class, get_val);
        ASSERT_TRUE(result_has_value(val));
        EXPECT_EQ(val.value(), 182376);
    }
    ctx->Release();
}

#include <asbind20/ext/stdstring.hpp>

TEST_F(asbind_test_suite, script_string)
{
    asIScriptEngine* engine = get_engine();
    asIScriptModule* m = engine->GetModule("test_script_string", asGM_ALWAYS_CREATE);

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
        auto result = asbind20::script_invoke<void>(ctx, m->GetFunctionByName("check_str"), std::ref(str));
        ASSERT_TRUE(result_has_value(result));
    }

    ctx->Release();
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
