#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/vocabulary.hpp>

TEST(test_invoke, script_class)
{
    using namespace asbind20;
    using asbind_test::result_has_value;

    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine, true);
    auto* m = engine->GetModule(
        "test_script_class", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    global<true>(engine)
        .function(
            "void throw_err()",
            []() -> void
            { throw std::runtime_error("err"); }
        );

    m->AddScriptSection(
        "test_invoke.as",
        "class my_class"
        "{"
        "int m_val;"
        "void set_val(int new_val) { m_val = new_val; }"
        "int get_val() const { return m_val; }"
        "int& get_val_ref() { return m_val; }"
        "int err() { throw_err(); return 42; }"
        "};"
    );
    ASSERT_GE(m->Build(), 0);

    AS_NAMESPACE_QUALIFIER asITypeInfo* my_class_t = m->GetTypeInfoByName("my_class");
    ASSERT_NE(my_class_t, nullptr);

    {
        asbind20::request_context ctx(engine);

        auto my_class = asbind20::instantiate_class(ctx, my_class_t);

        AS_NAMESPACE_QUALIFIER asIScriptFunction* set_val = my_class_t->GetMethodByDecl("void set_val(int)");
        asbind20::script_invoke<void>(ctx, my_class, set_val, 182375);

        AS_NAMESPACE_QUALIFIER asIScriptFunction* get_val = my_class_t->GetMethodByDecl("int get_val() const");
        auto val = asbind20::script_invoke<int>(ctx, my_class, get_val);

        ASSERT_TRUE(result_has_value(val));
        EXPECT_EQ(val.value(), 182375);

        AS_NAMESPACE_QUALIFIER asIScriptFunction* get_val_ref = my_class_t->GetMethodByDecl("int& get_val_ref()");
        auto val_ref = asbind20::script_invoke<int&>(ctx, my_class, get_val_ref);

        ASSERT_TRUE(result_has_value(val_ref));
        EXPECT_EQ(val_ref.value(), 182375);

        *val_ref = 182376;

        val = asbind20::script_invoke<int>(ctx, my_class, get_val);
        ASSERT_TRUE(result_has_value(val));
        EXPECT_EQ(val.value(), 182376);

        auto* err = my_class_t->GetMethodByDecl("int err()");
        ASSERT_TRUE(err);

        auto err_result = script_invoke<int>(ctx, err);
        EXPECT_FALSE(result_has_value(err_result));
        EXPECT_THROW((void)err_result.value(), bad_script_invoke_result_access);
    }
}
