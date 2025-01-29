#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>
#include <sstream>

using namespace asbind_test;

TEST(detail, generate_member_funcdef)
{
    using namespace asbind20;
    using detail::generate_member_funcdef;

    EXPECT_EQ(
        generate_member_funcdef("my_type", "void f()"),
        "void my_type::f()"
    );
    EXPECT_EQ(
        generate_member_funcdef("my_type", "void& f()"),
        "void& my_type::f()"
    );
    EXPECT_EQ(
        generate_member_funcdef("my_type", "void&f()"),
        "void& my_type::f()"
    );
    EXPECT_EQ(
        generate_member_funcdef("my_type", "int[]f()"),
        "int[] my_type::f()"
    );
}

TEST_F(asbind_test_suite, interface)
{
    asIScriptEngine* engine = get_engine();

    {
        asbind20::interface i(engine, "my_interface");
        i
            .funcdef("int callback(int)")
            .method("int get(callback@) const");

        EXPECT_EQ(i.get_engine(), engine);
    }

    asIScriptModule* m = engine->GetModule("test_interface", asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "test_interface.as",
        "class my_impl : my_interface"
        "{"
        "int get(my_interface::callback@ cb) const override { return cb(40); }"
        "};"
        "int add2(int val) { return val + 2; }"
        "int test() { my_impl val; return val.get(add2); }"
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

void out_str(std::string& out)
{
    out = "test";
}

std::string my_to_str(void* ref, int type_id)
{
    std::stringstream ss;
    if(asbind20::is_primitive_type(type_id))
    {
        asbind20::visit_primitive_type(
            [&ss]<typename T>(const T* ref)
            {
                if constexpr(std::same_as<T, bool>)
                {
                    ss << (*ref ? "true" : "false");
                }
                else
                    ss << *ref;
            },
            type_id,
            ref
        );
    }
    else
    {
        ss << type_id << " at " << ref;
    }

    return std::move(ss).str();
}

void my_to_str2(int prefix_num, void* ref, int type_id, std::string& out)
{
    std::string result;
    result = std::to_string(prefix_num);
    result += ": ";
    result += my_to_str(ref, type_id);

    out = std::move(result);
}

class member_var_type
{
public:
    member_var_type() = default;
    member_var_type(const member_var_type&) = default;

    ~member_var_type() = default;

    std::string mem_to_str1(void* ref, int type_id)
    {
        return my_to_str(ref, type_id);
    }

    void mem_to_str2(int prefix_num, void* ref, int type_id, std::string& out)
    {
        my_to_str2(prefix_num + m_prefix_offset, ref, type_id, out);
    }

private:
    int m_prefix_offset = -1;
};

// objfirst
std::string mem_to_str3(member_var_type& v, void* ref, int type_id)
{
    return v.mem_to_str1(ref, type_id);
}

// objfirst
void mem_to_str4(member_var_type& v, int prefix_num, void* ref, int type_id, std::string& out)
{
    return v.mem_to_str2(prefix_num, ref, type_id, out);
}

// objlast
std::string mem_to_str5(void* ref, int type_id, member_var_type& v)
{
    return v.mem_to_str1(ref, type_id);
}

// objlast
void mem_to_str6(int prefix_num, void* ref, int type_id, std::string& out, member_var_type& v)
{
    v.mem_to_str2(prefix_num, ref, type_id, out);
}
} // namespace test_bind

TEST_F(asbind_test_suite, generic_wrapper)
{
    using namespace asbind20;

    asIScriptEngine* engine = get_engine();

    asGENFUNC_t my_div_gen = to_asGENFUNC_t(fp<&test_bind::my_div>, call_conv<asCALL_CDECL>);
    asGENFUNC_t out_str_gen = to_asGENFUNC_t(fp<&test_bind::out_str>, call_conv<asCALL_CDECL>);
    asGENFUNC_t my_to_str_gen = to_asGENFUNC_t(fp<&test_bind::my_to_str>, call_conv<asCALL_CDECL>, var_type<0>);
    asGENFUNC_t my_to_str2_gen = to_asGENFUNC_t(fp<&test_bind::my_to_str2>, call_conv<asCALL_CDECL>, var_type<1>);

    using test_bind::member_var_type;
    asGENFUNC_t mem_to_str1 = to_asGENFUNC_t(fp<&member_var_type::mem_to_str1>, call_conv<asCALL_THISCALL>, var_type<0>);
    asGENFUNC_t mem_to_str2 = to_asGENFUNC_t(fp<&member_var_type::mem_to_str2>, call_conv<asCALL_THISCALL>, var_type<1>);
    asGENFUNC_t mem_to_str3 = to_asGENFUNC_t(fp<&test_bind::mem_to_str3>, call_conv<asCALL_CDECL_OBJFIRST>, var_type<0>);
    asGENFUNC_t mem_to_str4 = to_asGENFUNC_t(fp<&test_bind::mem_to_str4>, call_conv<asCALL_CDECL_OBJFIRST>, var_type<1>);
    asGENFUNC_t mem_to_str5 = to_asGENFUNC_t(fp<&test_bind::mem_to_str5>, call_conv<asCALL_CDECL_OBJLAST>, var_type<0>);
    asGENFUNC_t mem_to_str6 = to_asGENFUNC_t(fp<&test_bind::mem_to_str6>, call_conv<asCALL_CDECL_OBJLAST>, var_type<1>);

    {
        constexpr auto result = detail::gen_script_arg_idx<2>(var_type<0>);
        static_assert(result.size() == 2);

        static_assert(result[0] == 0);
        static_assert(result[1] == 0);

        static_assert(!detail::var_type_tag<var_type_t<0>, 0>{});
        static_assert(detail::var_type_tag<var_type_t<0>, 1>{});
    }

    {
        constexpr auto result = detail::gen_script_arg_idx<4>(var_type<1>);
        static_assert(result.size() == 4);

        static_assert(result[0] == 0);
        static_assert(result[1] == 1);
        static_assert(result[2] == 1);
        static_assert(result[3] == 2);

        static_assert(!detail::var_type_tag<var_type_t<1>, 0>{});
        static_assert(!detail::var_type_tag<var_type_t<1>, 1>{});
        static_assert(detail::var_type_tag<var_type_t<1>, 2>{});
        static_assert(!detail::var_type_tag<var_type_t<1>, 3>{});
    }

    global<true>(engine)
        .function("int my_div(int a, int b)", my_div_gen)
        .function("void out_str(string&out)", out_str_gen)
        .function("string my_to_str(const ?&in)", my_to_str_gen)
        .function("void my_to_str2(int prefix_num, const ?&in, string&out)", my_to_str2_gen);

    value_class<member_var_type, true>(
        engine, "member_var_type", asOBJ_APP_CLASS_ALLINTS
    )
        .behaviours_by_traits()
        .method("string to_str1(const ?&in)", mem_to_str1)
        .method("void to_str2(int prefix_num, const ?&in, string&out)", mem_to_str2)
        .method("string to_str3(const ?&in)", mem_to_str3)
        .method("void to_str4(int prefix_num, const ?&in, string&out)", mem_to_str4)
        .method("string to_str5(const ?&in)", mem_to_str5)
        .method("void to_str6(int prefix_num, const ?&in, string&out)", mem_to_str6);

    asIScriptModule* m = engine->GetModule("test_generic", asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "test_def.as",
        "void main()"
        "{"
        "assert(my_div(6, 2) == 3);"
        "assert(my_to_str(true) == \"true\");"
        "assert(my_to_str(6) == \"6\");"
        "string result;"
        "out_str(result);"
        "assert(result == \"test\");"
        "my_to_str2(1, false, result);"
        "assert(result == \"1: false\");"
        "}"
        "void test_member()\n"
        "{\n"
        "string result;\n"
        "member_var_type v;\n"
        "assert(v.to_str1(1013) == \"1013\");\n"
        "assert(v.to_str3(1013) == \"1013\");\n"
        "assert(v.to_str5(1013) == \"1013\");\n"
        "v.to_str2(1, false, result);\n"
        "assert(result == \"0: false\");\n"
        "v.to_str4(1, false, result);\n"
        "assert(result == \"0: false\");\n"
        "v.to_str6(1, false, result);\n"
        "assert(result == \"0: false\");\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    {
        request_context ctx(engine);
        asIScriptFunction* func = m->GetFunctionByDecl("void main()");
        ASSERT_TRUE(func);
        auto result = script_invoke<void>(ctx, func);
        EXPECT_TRUE(result_has_value(result));
    }

    {
        request_context ctx(engine);
        asIScriptFunction* func = m->GetFunctionByDecl("void test_member()");
        ASSERT_TRUE(func);
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

TEST_F(asbind_test_suite, enum_helper)
{
#ifndef ASBIND20_HAS_STATIC_ENUM_NAME
    GTEST_SKIP() << "ASBIND20_HAS_STATIC_ENUM_NAME not defined";

#else
    asIScriptEngine* engine = get_engine();

    using test_bind::my_enum;
    {
        asbind20::enum_<my_enum> e(engine, "my_enum");
        e
            .value(my_enum::A, "A")
            .value<my_enum::B>();

        EXPECT_EQ(e.get_engine(), engine);
    }

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
#endif
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    std::cerr << "asGetLibraryVersion(): " << asGetLibraryVersion() << std::endl;
    std::cerr << "asGetLibraryOptions(): " << asGetLibraryOptions() << std::endl;
    std::cerr << "asbind20::library_version(): " << asbind20::library_version() << std::endl;
    return RUN_ALL_TESTS();
}
