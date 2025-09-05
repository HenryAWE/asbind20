#include <shared_test_lib.hpp>

namespace test_bind
{
void register_int_array(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    using arr_type = int[4];

    value_class<arr_type>(engine, "int_arr")
        .constructor_function(
            "",
            [](arr_type* a)
            {
                new(a) arr_type{1, 2};
            }
        )
        .destructor()
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

    value_class<arr_type, true>(engine, "int_arr")
        .constructor_function(
            "",
            [](arr_type* a)
            {
                new(a) arr_type{1, 2};
            }
        )
        .destructor()
        .method(
            "int& opIndex(uint idx)",
            [](arr_type& arr, std::uint32_t idx) -> int&
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
        "int test()\n"
        "{\n"
        "    int_arr a;\n"
        //"    a[0] = 1; a[1] = 2;\n"
        "    return a[0] + a[1];\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    auto* f = m->GetFunctionByName("test");
    asbind20::request_context ctx(engine);

    auto result = asbind20::script_invoke<int>(ctx, f);
    EXPECT_TRUE(asbind_test::result_has_value(result));
    EXPECT_EQ(result.value(), 1 + 2);
}
} // namespace test_bind

TEST(test_c_array, native)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    test_bind::register_int_array(engine);
    test_bind::check_int_array(engine);
}

TEST(test_c_array, generic)
{
    auto engine = asbind20::make_script_engine();
    asbind_test::setup_message_callback(engine, true);

    test_bind::register_int_array(asbind20::use_generic, engine);
    test_bind::check_int_array(engine);
}
