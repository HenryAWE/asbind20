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
    EXPECT_EQ(
        generate_member_funcdef("my_type", "int@[]f()"),
        "int@[] my_type::f()"
    );
    EXPECT_EQ(
        generate_member_funcdef("my_type", "int[]@f()"),
        "int[]@ my_type::f()"
    );

    EXPECT_EQ(
        generate_member_funcdef("my_type", "container::list@ f()"),
        "container::list@ my_type::f()"
    );
    EXPECT_EQ(
        generate_member_funcdef("my_type", "container::list_iterator f()"),
        "container::list_iterator my_type::f()"
    );
}

TEST_F(asbind_test_suite, interface)
{
    auto* engine = get_engine();

    {
        asbind20::interface i(engine, "my_interface");
        i
            .funcdef("int callback(int)")
            .method("int get(callback@) const");

        EXPECT_EQ(i.get_engine(), engine);
    }

    auto* m = engine->GetModule(
        "test_interface", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );

    m->AddScriptSection(
        "test_interface.as",
        "class my_impl : my_interface\n"
        "{\n"
        "    int get(my_interface::callback@ cb) const override { return cb(40); }\n"
        "};\n"
        "int add2(int val) { return val + 2; }\n"
        "int test() { my_impl val; return val.get(add2); }"
    );
    ASSERT_GE(m->Build(), 0);

    {
        auto* func = m->GetFunctionByDecl("int test()");
        ASSERT_TRUE(func);

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, func);
        ASSERT_TRUE(result_has_value(result));
        EXPECT_EQ(result.value(), 42);
    }

    m->Discard();
}

TEST_F(asbind_test_suite, funcdef_and_typedef)
{
    auto* engine = get_engine();

    asbind20::global(engine)
        .funcdef("bool callback(int, int)")
        .typedef_("float", "real32")
        .using_("float32", "float");

    auto* m = engine->GetModule(
        "test_def", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );

    m->AddScriptSection(
        "test_def.as",
        "bool pred(int a, int b) { return a < b; }\n"
        "void main() { callback@ cb = @pred; assert(cb(1, 2)); }\n"
        "real32 get_pi() { return 3.14f; }\n"
        "float32 get_pi_2() { return 3.14f; }"
    );
    ASSERT_GE(m->Build(), 0);

    {
        asbind20::request_context ctx(engine);
        auto* func = m->GetFunctionByDecl("void main()");
        ASSERT_TRUE(func);

        auto result = asbind20::script_invoke<void>(ctx, func);
        ASSERT_TRUE(result_has_value(result));
    }

    {
        asbind20::request_context ctx(engine);
        auto* func = m->GetFunctionByDecl("real32 get_pi()");
        ASSERT_TRUE(func);

        auto result = asbind20::script_invoke<float>(ctx, func);
        ASSERT_TRUE(result_has_value(result));
        EXPECT_FLOAT_EQ(result.value(), 3.14f);
    }

    {
        asbind20::request_context ctx(engine);
        auto* func = m->GetFunctionByDecl("float32 get_pi_2()");
        ASSERT_TRUE(func);

        auto result = asbind20::script_invoke<float>(ctx, func);
        ASSERT_TRUE(result_has_value(result));
        EXPECT_FLOAT_EQ(result.value(), 3.14f);
    }

    m->Discard();
}

namespace test_bind
{
static int my_div(int a, int b)
{
    return a / b;
}

// AngelScript decl: int my_mul(int a, int b)
static void my_mul(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
{
    int a = asbind20::get_generic_arg<int>(gen, 0);
    int b = asbind20::get_generic_arg<int>(gen, 1);
    asbind20::set_generic_return<int>(gen, a * b);
}

static void out_str(std::string& out)
{
    out = "test";
}

static std::string my_to_str(void* ref, int type_id)
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

static void my_to_str2(int prefix_num, void* ref, int type_id, std::string& out)
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
static std::string mem_to_str3(member_var_type& v, void* ref, int type_id)
{
    return v.mem_to_str1(ref, type_id);
}

// objfirst
static void mem_to_str4(member_var_type& v, int prefix_num, void* ref, int type_id, std::string& out)
{
    return v.mem_to_str2(prefix_num, ref, type_id, out);
}

// objlast
static std::string mem_to_str5(void* ref, int type_id, member_var_type& v)
{
    return v.mem_to_str1(ref, type_id);
}

// objlast
static void mem_to_str6(int prefix_num, void* ref, int type_id, std::string& out, member_var_type& v)
{
    v.mem_to_str2(prefix_num, ref, type_id, out);
}

consteval void test_detail_arg_idx()
{
    using asbind20::var_type_t, asbind20::var_type;
    using asbind20::detail::gen_script_arg_idx;
    using asbind20::detail::var_type_tag;

    {
        // C++: (void*, int)
        // AS: (?)
        constexpr auto result = gen_script_arg_idx<2>(var_type<0>);
        static_assert(result.size() == 2);

        static_assert(result[0] == 0);
        static_assert(result[1] == 0);

        static_assert(!var_type_tag<var_type_t<0>, 0>{});
        static_assert(var_type_tag<var_type_t<0>, 1>{});
    }

    {
        // C++: (type, void*, int, type)
        // AS: (type, ?, type)
        constexpr auto result = gen_script_arg_idx<4>(var_type<1>);
        static_assert(result.size() == 4);

        static_assert(result[0] == 0);
        static_assert(result[1] == 1);
        static_assert(result[2] == 1);
        static_assert(result[3] == 2);

        static_assert(!var_type_tag<var_type_t<1>, 0>{});
        static_assert(!var_type_tag<var_type_t<1>, 1>{});
        static_assert(var_type_tag<var_type_t<1>, 2>{});
        static_assert(!var_type_tag<var_type_t<1>, 3>{});
    }

    {
        // C++: (void*, int, type, void*, int)
        // AS: (?, type, ?)
        constexpr auto result = gen_script_arg_idx<5>(var_type<0, 2>);
        static_assert(result.size() == 5);

        static_assert(result[0] == 0);
        static_assert(result[1] == 0);
        static_assert(result[2] == 1);
        static_assert(result[3] == 2);
        static_assert(result[4] == 2);

        static_assert(!var_type_tag<var_type_t<0, 2>, 0>{});
        static_assert(var_type_tag<var_type_t<0, 2>, 1>{});
        static_assert(!var_type_tag<var_type_t<0, 2>, 2>{});
        static_assert(var_type_tag<var_type_t<0, 2>, 3>{});
        static_assert(!var_type_tag<var_type_t<0, 2>, 4>{});
    }
}
} // namespace test_bind

TEST_F(asbind_test_suite, generic_wrapper)
{
    using namespace asbind20;

    auto* engine = get_engine();

    auto my_div_gen = to_asGENFUNC_t(fp<&test_bind::my_div>, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>);
    auto my_mul_gen = to_asGENFUNC_t(test_bind::my_mul, call_conv<AS_NAMESPACE_QUALIFIER asCALL_GENERIC>);
    auto my_add_gen = to_asGENFUNC_t(
        [](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
        {
            int a = asbind20::get_generic_arg<int>(gen, 0);
            int b = asbind20::get_generic_arg<int>(gen, 1);
            asbind20::set_generic_return<int>(gen, a + b);
        },
        generic_call_conv
    );

    auto out_str_gen = to_asGENFUNC_t(fp<&test_bind::out_str>, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>);
    auto my_to_str_gen = to_asGENFUNC_t(fp<&test_bind::my_to_str>, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>, var_type<0>);
    auto my_to_str2_gen = to_asGENFUNC_t(fp<&test_bind::my_to_str2>, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL>, var_type<1>);

    using test_bind::member_var_type;
    auto mem_to_str1 = to_asGENFUNC_t(fp<&member_var_type::mem_to_str1>, call_conv<AS_NAMESPACE_QUALIFIER asCALL_THISCALL>, var_type<0>);
    auto mem_to_str2 = to_asGENFUNC_t(fp<&member_var_type::mem_to_str2>, call_conv<AS_NAMESPACE_QUALIFIER asCALL_THISCALL>, var_type<1>);
    auto mem_to_str3 = to_asGENFUNC_t(fp<&test_bind::mem_to_str3>, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>, var_type<0>);
    auto mem_to_str4 = to_asGENFUNC_t(fp<&test_bind::mem_to_str4>, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>, var_type<1>);
    auto mem_to_str5 = to_asGENFUNC_t(fp<&test_bind::mem_to_str5>, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>, var_type<0>);
    auto mem_to_str6 = to_asGENFUNC_t(fp<&test_bind::mem_to_str6>, call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>, var_type<1>);

    global<true>(engine)
        .function("int my_div(int a, int b)", my_div_gen)
        .function("int my_mul(int a, int b)", my_mul_gen)
        .function("int my_add(int a, int b)", my_add_gen)
        .function("void out_str(string&out)", out_str_gen)
        .function("string my_to_str(const ?&in)", my_to_str_gen)
        .function("void my_to_str2(int prefix_num, const ?&in, string&out)", my_to_str2_gen);

    value_class<member_var_type, true>(
        engine, "member_var_type", AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS
    )
        .behaviours_by_traits()
        .method("string to_str1(const ?&in)", mem_to_str1)
        .method("void to_str2(int prefix_num, const ?&in, string&out)", mem_to_str2)
        .method("string to_str3(const ?&in)", mem_to_str3)
        .method("void to_str4(int prefix_num, const ?&in, string&out)", mem_to_str4)
        .method("string to_str5(const ?&in)", mem_to_str5)
        .method("void to_str6(int prefix_num, const ?&in, string&out)", mem_to_str6);

    auto* m = engine->GetModule(
        "test_generic", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );

    m->AddScriptSection(
        "test_def.as",
        "void main()\n"
        "{\n"
        "    assert(my_div(6, 2) == 3);\n"
        "    assert(my_mul(2, 3) == 6);\n"
        "    assert(my_add(2, 3) == 5);\n"
        "    assert(my_to_str(true) == \"true\");\n"
        "    assert(my_to_str(6) == \"6\");\n"
        "    string result;\n"
        "    out_str(result);\n"
        "    assert(result == \"test\");\n"
        "    my_to_str2(1, false, result);\n"
        "    assert(result == \"1: false\");\n"
        "}\n"
        "void test_member()\n"
        "{\n"
        "    string result;\n"
        "    member_var_type v;\n"
        "    assert(v.to_str1(1013) == \"1013\");\n"
        "    assert(v.to_str3(1013) == \"1013\");\n"
        "    assert(v.to_str5(1013) == \"1013\");\n"
        "    v.to_str2(1, false, result);\n"
        "    assert(result == \"0: false\");\n"
        "    v.to_str4(1, false, result);\n"
        "    assert(result == \"0: false\");\n"
        "    v.to_str6(1, false, result);\n"
        "    assert(result == \"0: false\");\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    {
        request_context ctx(engine);
        auto* func = m->GetFunctionByDecl("void main()");
        ASSERT_TRUE(func);
        auto result = script_invoke<void>(ctx, func);
        EXPECT_TRUE(result_has_value(result));
    }

    {
        request_context ctx(engine);
        auto* func = m->GetFunctionByDecl("void test_member()");
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

TEST_F(asbind_test_suite, enum_)
{
    auto* engine = get_engine();

    using test_bind::my_enum;
    {
        asbind20::enum_<my_enum> e(engine, "my_enum");
#ifndef ASBIND20_HAS_STATIC_ENUM_NAME
        e
            .value(my_enum::A, "A")
            .value(my_enum::B, "B");

#else
        e
            .value(my_enum::A, "A")
            .value<my_enum::B>(); // test auto-generated name

        EXPECT_EQ(e.get_engine(), engine);
        EXPECT_EQ(e.get_name(), "my_enum");
#endif
    }

    auto* m = engine->GetModule(
        "test_enum", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );

    m->AddScriptSection(
        "test_enum.as",
        "my_enum get_enum_val() { return my_enum::A; }\n"
        "bool check_enum_val(my_enum val) { return val == my_enum::B; }"
    );
    ASSERT_GE(m->Build(), 0);

    {
        asbind20::request_context ctx(engine);
        auto* func = m->GetFunctionByDecl("my_enum get_enum_val()");
        ASSERT_TRUE(func);

        auto result = asbind20::script_invoke<my_enum>(ctx, func);
        ASSERT_TRUE(result_has_value(result));
        EXPECT_EQ(result.value(), my_enum::A);
    }

    {
        asbind20::request_context ctx(engine);
        auto* func = m->GetFunctionByDecl("bool check_enum_val(my_enum val)");
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

    std::cerr << "__cplusplus = " << __cplusplus << 'L' << std::endl;
    std::cerr << "asGetLibraryVersion(): " << AS_NAMESPACE_QUALIFIER asGetLibraryVersion() << std::endl;
    std::cerr << "asGetLibraryOptions(): " << AS_NAMESPACE_QUALIFIER asGetLibraryOptions() << std::endl;
    std::cerr << "asbind20::library_version(): " << asbind20::library_version() << std::endl;

#ifdef ASBIND20_HAS_AS_INITIALIZER_LIST
    std::cerr << "ASBIND20_HAS_AS_INITIALIZER_LIST: " << ASBIND20_HAS_AS_INITIALIZER_LIST << std::endl;
#else
    std::cerr << "ASBIND20_HAS_AS_INITIALIZER_LIST not defined" << std::endl;
#endif

    return RUN_ALL_TESTS();
}
