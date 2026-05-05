#include <asbind_test/framework.hpp>

namespace test_bind
{
struct val_class_for_appending
{
    int data = 0;

    int get_data() const
    {
        return data;
    }
};
} // namespace test_bind

TEST(Appending, ValueClass)
{
    using test_bind::val_class_for_appending;
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    value_class<val_class_for_appending, true>(engine, "val")
        .behaviours_by_traits()
        .method("int get_data() const", fp<&val_class_for_appending::get_data>);

    value_class<val_class_for_appending, true>(appending, engine, "val")
        .property("int data", &val_class_for_appending::data);

    auto* m = engine->GetModule(
        "appending", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    ASSERT_NE(m, nullptr);
    m->AddScriptSection(
        "appending",
        "int f()\n"
        "{\n"
        "    val v;\n"
        "    v.data = 1013;\n"
        "    return v.get_data();\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByDecl("int f()");
    ASSERT_NE(f, nullptr);

    request_context ctx(engine);
    auto result = script_invoke<int>(ctx, f);
    ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
    EXPECT_EQ(result.value(), 1013);
}
