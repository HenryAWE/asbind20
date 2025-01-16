#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>

using namespace asbind_test;

TEST_F(asbind_test_suite, interface)
{
    asIScriptEngine* engine = get_engine();

    asbind20::interface(engine, "my_interface")
        .method("int get() const");

    asIScriptModule* m = engine->GetModule("test_interface", asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "test_interface.as",
        "class my_impl : my_interface"
        "{"
        "int get() const override { return 42; }"
        "};"
        "int test() { my_impl val; return val.get(); }"
    );
    ASSERT_GE(m->Build(), 0);

    asIScriptContext* ctx = engine->CreateContext();
    asIScriptFunction* func = m->GetFunctionByDecl("int test()");
    ASSERT_TRUE(func);

    auto result = asbind20::script_invoke<int>(ctx, func);
    ASSERT_TRUE(result_has_value(result));
    EXPECT_EQ(result.value(), 42);

    ctx->Release();

    m->Discard();
}

TEST_F(asbind_test_suite, funcdef_and_typedef)
{
    asIScriptEngine* engine = get_engine();

    asbind20::global(engine)
        .funcdef("bool callback(int, int)")
        .typedef_("float", "real32")
        .using_("float32", "float");

    asIScriptModule* m = engine->GetModule("test_def", asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "test_def.as",
        "bool pred(int a, int b) { return a < b; }"
        "void main() { callback@ cb = @pred; assert(cb(1, 2)); }"
        "real32 get_pi() { return 3.14f; }"
        "float32 get_pi_2() { return 3.14f; }"
    );
    ASSERT_GE(m->Build(), 0);

    {
        asbind20::request_context ctx(engine);
        asIScriptFunction* func = m->GetFunctionByDecl("void main()");
        ASSERT_TRUE(func);

        auto result = asbind20::script_invoke<void>(ctx, func);
        ASSERT_TRUE(result_has_value(result));
    }

    {
        asbind20::request_context ctx(engine);
        asIScriptFunction* func = m->GetFunctionByDecl("real32 get_pi()");
        ASSERT_TRUE(func);

        auto result = asbind20::script_invoke<float>(ctx, func);
        ASSERT_TRUE(result_has_value(result));
        EXPECT_FLOAT_EQ(result.value(), 3.14f);
    }

    {
        asbind20::request_context ctx(engine);
        asIScriptFunction* func = m->GetFunctionByDecl("float32 get_pi_2()");
        ASSERT_TRUE(func);

        auto result = asbind20::script_invoke<float>(ctx, func);
        ASSERT_TRUE(result_has_value(result));
        EXPECT_FLOAT_EQ(result.value(), 3.14f);
    }

    m->Discard();
}

namespace test_bind
{
int my_div(int a, int b)
{
    return a / b;
}
} // namespace test_bind

TEST_F(asbind_test_suite, generic_wrapper)
{
    using namespace asbind20;

    asIScriptEngine* engine = get_engine();

    asGENFUNC_t my_div_gen = generic_wrapper<&test_bind::my_div, asCALL_CDECL>();

    global<true>(engine)
        .function("int my_div(int a, int b)", my_div_gen);

    asIScriptModule* m = engine->GetModule("test_generic", asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "test_def.as",
        "void main()"
        "{"
        "assert(my_div(6, 2) == 3);"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    {
        request_context ctx(engine);
        asIScriptFunction* func = m->GetFunctionByDecl("void main()");
        auto result = script_invoke<void>(ctx, func);
        EXPECT_TRUE(result_has_value(result));
    }

    m->Discard();
}

namespace test_bind
{
enum class my_enum : int
{
    A,
    B
};
} // namespace test_bind

TEST_F(asbind_test_suite, enum)
{
    asIScriptEngine* engine = get_engine();

    using test_bind::my_enum;

    asbind20::global(engine)
        .enum_type("my_enum")
        .enum_value("my_enum", my_enum::A, "A")
        .enum_value("my_enum", my_enum::B, "B");

    asIScriptModule* m = engine->GetModule("test_enum", asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "test_enum.as",
        "my_enum get_enum_val() { return my_enum::A; }"
        "bool check_enum_val(my_enum val) { return val == my_enum::B; }"
    );
    ASSERT_GE(m->Build(), 0);

    {
        asbind20::request_context ctx(engine);
        asIScriptFunction* func = m->GetFunctionByDecl("my_enum get_enum_val()");
        ASSERT_TRUE(func);

        auto result = asbind20::script_invoke<my_enum>(ctx, func);
        ASSERT_TRUE(result_has_value(result));
        EXPECT_EQ(result.value(), my_enum::A);
    }

    {
        asbind20::request_context ctx(engine);
        asIScriptFunction* func = m->GetFunctionByDecl("bool check_enum_val(my_enum val)");
        ASSERT_TRUE(func);

        auto result = asbind20::script_invoke<bool>(ctx, func, my_enum::B);
        ASSERT_TRUE(result_has_value(result));
        EXPECT_TRUE(result.value());
    }

    m->Discard();
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    std::cerr << "asGetLibraryVersion(): " << asGetLibraryVersion() << std::endl;
    std::cerr << "asGetLibraryOptions(): " << asGetLibraryOptions() << std::endl;
    std::cerr << "asbind20::library_version(): " << asbind20::library_version() << std::endl;
    return RUN_ALL_TESTS();
}
