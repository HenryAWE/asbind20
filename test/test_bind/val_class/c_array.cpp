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

static void check_int_array(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
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
        "    int_arr copied(a);\n"
        "    return a[0] + a[1];\n"
        "}\n"
    );
    ASSERT_GE(m->Build(), 0);

    {
        auto* f = m->GetFunctionByName("test0");
        ASSERT_NE(f, nullptr);
        asbind20::request_context ctx(engine);

        auto result = asbind20::script_invoke<int>(ctx, f);
        EXPECT_TRUE(asbind_test::result_has_value(result));
        EXPECT_EQ(result.value(), 1 + 2);
    }

    {
        auto* f = m->GetFunctionByName("test1");
        ASSERT_NE(f, nullptr);
        asbind20::request_context ctx(engine);

        auto result = asbind20::script_invoke<int>(ctx, f);
        EXPECT_TRUE(asbind_test::result_has_value(result));
        EXPECT_EQ(result.value(), 3 + 4);
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
