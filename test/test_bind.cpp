#include <gtest/gtest.h>
#include "shared.hpp"
#include <asbind20/asbind.hpp>

namespace test_bind
{
class my_value_class
{
public:
    my_value_class() = default;
    my_value_class(const my_value_class&) = default;

    ~my_value_class() = default;

    my_value_class& operator=(const my_value_class&) = default;

    int get_val() const
    {
        return value;
    }

    void set_val(int new_val)
    {
        value = new_val;
    }

    int value = 0;
};
} // namespace test_bind

using asbind_test::asbind_test_suite;

TEST_F(asbind_test_suite, value_class)
{
    asIScriptEngine* engine = get_engine();

    using test_bind::my_value_class;

    asbind20::value_class<my_value_class>(engine, "my_value_class")
        .register_basic_methods()
        .method("void set_val(int)", &my_value_class::set_val)
        .method("int get_val() const", &my_value_class::get_val);

    asIScriptModule* m = engine->GetModule("test_value_class", asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "test_value_class.as",
        "int test()"
        "{"
        "my_value_class val;"
        "val.set_val(42);"
        "return val.get_val();"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    asIScriptContext* ctx = engine->CreateContext();
    {
        auto test = asbind20::script_function<int()>(m->GetFunctionByName("test"));
        EXPECT_EQ(test(ctx), 42);
    }
    ctx->Release();
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
