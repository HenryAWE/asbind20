#include <asbind_test/framework.hpp>

namespace test_bind
{
static void register_int_array(asbind20::engine_pointer engine)
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
            },
            objfirst
        )
        .copy_constructor()
        .method(
            "int& opIndex(uint idx)",
            [](arr_type& arr, std::uint32_t idx) -> int&
            { return arr[idx]; }
        );
}

static void register_int_array(
    asbind20::use_generic_t, asbind20::engine_pointer engine
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
static void register_string_array(asbind20::engine_pointer engine)
{
    asbind_test::setup_script_string(engine, UseGeneric);
    asbind_test::setup_script_assertion(engine);

    using namespace asbind20;

    using std::string;
    using arr_type = string[3];

    asbind20::value_class<arr_type, UseGeneric>(engine, "str_arr")
        .default_constructor()
        .copy_constructor()
        .constructor_function(
            "const string&in val",
            [](arr_type* a, const string& s)
            {
                arr_type& arr = *a;
                new(arr + 0) std::string(s);
                new(arr + 1) std::string(s);
                new(arr + 2) std::string("!");
            }
        )
        .constructor_function(
            "int iv",
            [](arr_type* a, int iv)
            {
                arr_type& arr = *a;

                std::string s = std::to_string(iv);
                new(arr + 0) std::string(s);
                new(arr + 1) std::string(s);
                new(arr + 2) std::string("!");
            }
        )
        .destructor()
        .method(
            "const string& opIndex(uint idx) const",
            [](const arr_type& arr, uint32_t idx) -> const string&
            { return arr[idx]; }
        );
}

static void check_int_array(asbind20::engine_pointer engine)
{
    auto* m = asbind20::create_module(engine, "test_int_array");
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
        "    int_arr copied(a);\n"
        "    return a[0] + a[1];\n"
        "}\n"
    );
    ASSERT_GE(m->Build(), 0);

    {
        SCOPED_TRACE("test0");

        auto* f = m->GetFunctionByName("test0");
        ASSERT_THAT(f, ::testing::NotNull());

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, f);
        ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
        EXPECT_EQ(result.value(), 1 + 2);
    }

    {
        SCOPED_TRACE("test1");

        auto* f = m->GetFunctionByName("test1");
        ASSERT_THAT(f, ::testing::NotNull());

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, f);
        ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
        EXPECT_EQ(result.value(), 3 + 4);
    }
}

static void check_string_array(asbind20::engine_pointer engine)
{
    auto* m = asbind20::create_module(
        engine, "test_string_array"
    );
    ASSERT_THAT(m, ::testing::NotNull());
    m->AddScriptSection(
        "test_string_array",
        "string test0()\n"
        "{\n"
        "    str_arr s(1013);\n"
        "    assert(s[2] == \"!\");\n"
        "    return s[0];\n"
        "}"
        "string test1()\n"
        "{\n"
        "    str_arr s(\"test\");\n"
        "    assert(s[2] == \"!\");\n"
        "    return s[0];\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    {
        SCOPED_TRACE("test0");

        auto* f = m->GetFunctionByName("test0");
        ASSERT_THAT(f, ::testing::NotNull());

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<std::string>(ctx, f);
        ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
        EXPECT_EQ(result.value(), "1013");
    }

    {
        SCOPED_TRACE("test1");

        auto* f = m->GetFunctionByName("test1");
        ASSERT_THAT(f, ::testing::NotNull());

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<std::string>(ctx, f);
        ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
        EXPECT_EQ(result.value(), "test");
    }
}
} // namespace test_bind

TEST(TestCArray, Native)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine);

    test_bind::register_int_array(engine);
    test_bind::check_int_array(engine);
}

TEST(TestCArray, Generic)
{
    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine);

    test_bind::register_int_array(asbind20::use_generic, engine);
    test_bind::check_int_array(engine);
}

TEST(TestStringCArray, Native)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine);

    test_bind::register_string_array<false>(engine);
    test_bind::check_string_array(engine);
}

TEST(TestStringCArray, Generic)
{
    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine);

    test_bind::register_string_array<true>(engine);
    test_bind::check_string_array(engine);
}
