#include <asbind_test/framework.hpp>

namespace test_bind
{
void register_int_array(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    using arr_type = int[4];

    value_class<arr_type>(
        engine, "int_arr", AS_NAMESPACE_QUALIFIER asOBJ_POD
    )
        .default_constructor()
        .constructor_function(
            "int val",
            [](arr_type* a, int val)
            {
                new(a) arr_type{val, val};
            }
        )
        .copy_constructor()
        .method(
            "int& opIndex(uint idx)",
            [](arr_type& arr, std::uint32_t idx) -> int&
            { return arr[idx]; }
        );
}

void register_int_array(
    asbind20::use_generic_t, AS_NAMESPACE_QUALIFIER asIScriptEngine* engine
)
{
    using namespace asbind20;

    using arr_type = int[4];

    value_class<arr_type, true>(
        engine, "int_arr", AS_NAMESPACE_QUALIFIER asOBJ_POD
    )
        .default_constructor()
        .constructor_function(
            "int val",
            [](arr_type* a, int val)
            {
                new(a) arr_type{val, val};
            }
        )
        .copy_constructor()
        .method(
            "int& opIndex(uint idx)",
            [](arr_type& arr, std::uint32_t idx) -> int&
            { return arr[idx]; }
        );
}

template <bool UseGeneric>
void register_string_array(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    asbind_test::setup_script_string(engine, UseGeneric);

    using namespace asbind20;

    using std::string;
    using arr_type = string[3];

    asbind20::value_class<arr_type>(engine, "str_arr")
        .default_constructor()
        .copy_constructor()
        .constructor_function(
            "const string&in val",
            [](arr_type* a, const string& val)
            {
                new(a) arr_type{{val}, {val}, {"!"}};
            }
        )
        .constructor_function(
            "int iv",
            [](arr_type* a, int iv)
            {
                std::string s = std::to_string(iv);
                new(a) arr_type{{s}, {s}, {"!"}};
            }
        )
        .destructor()
        .method(
            "const string& opIndex(uint idx) const",
            [](const arr_type& arr, uint32_t idx) -> const string&
            { return arr[idx]; }
        );
}

void check_int_array(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = engine->GetModule(
        "test_int_array", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    m->AddScriptSection(
        "test_int_array",
        "int test0()\n"
        "{\n"
        "    int_arr a(1);\n"
        "    a[1] = 2;\n"
        "    return a[0] + a[1];\n"
        "}\n"
        "int test1()\n"
        "{\n"
        "    int_arr a(0);\n"
        "    a[0] = 3; a[1] = 4;\n"
        "    int_arr copied(a);"
        "    return a[0] + a[1];\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    {
        auto* f = m->GetFunctionByName("test0");
        ASSERT_TRUE(f);
        asbind20::request_context ctx(engine);

        auto result = asbind20::script_invoke<int>(ctx, f);
        EXPECT_TRUE(asbind_test::result_has_value(result));
        EXPECT_EQ(result.value(), 1 + 2);
    }

    {
        auto* f = m->GetFunctionByName("test1");
        ASSERT_TRUE(f);
        asbind20::request_context ctx(engine);

        auto result = asbind20::script_invoke<int>(ctx, f);
        EXPECT_TRUE(asbind_test::result_has_value(result));
        EXPECT_EQ(result.value(), 3 + 4);
    }
}

void check_string_array(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using std::string;

    auto* m = engine->GetModule(
        "test_string_array", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    m->AddScriptSection(
        "test_string_array",
        "uint test0()\n"
        "{\n"
        "    str_arr s(12);\n"
        "    return s[0].size;\n"
        "}"
        "uint test1()\n"
        "{\n"
        "    str_arr s(\"test\");\n"
        "    return s[0].size;\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    {
        auto* f = m->GetFunctionByName("test0");
        ASSERT_NE(f, nullptr);

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<std::uint32_t>(ctx, f);
        ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
        EXPECT_EQ(result.value(), 2); // "12"
    }

    {
        auto* f = m->GetFunctionByName("test1");
        ASSERT_NE(f, nullptr);

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<std::uint32_t>(ctx, f);
        ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
        EXPECT_EQ(result.value(), 4); // "test"
    }
}
} // namespace test_bind

TEST(TestCArray, Native)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    test_bind::register_int_array(engine);
    test_bind::check_int_array(engine);
}

TEST(TestCArray, Generic)
{
    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    test_bind::register_int_array(asbind20::use_generic, engine);
    test_bind::check_int_array(engine);
}

TEST(TestStringCArray, Native)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    test_bind::register_string_array<false>(engine);
    test_bind::check_string_array(engine);
}

TEST(TestStringCArray, Generic)
{
    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    test_bind::register_string_array<true>(engine);
    test_bind::check_string_array(engine);
}
