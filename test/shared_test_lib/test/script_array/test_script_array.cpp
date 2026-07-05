#include "test_script_array.hpp"

namespace test_script_array
{
void run_string(
    asbind20::engine_pointer engine,
    const char* section,
    std::string_view code
)
{
    run_string<void>(engine, section, code, "void");
}
} // namespace test_script_array

using TestArrayNative = test_script_array::basic_array_suite<false>;
using TestArrayGeneric = test_script_array::basic_array_suite<true>;

namespace test_script_array
{
using asbind20::compat::script_enum_value_type;

enum my_enum : script_enum_value_type
{
    neg_one = -1,
    zero = 0,
    one = 1,
    huge_val = std::numeric_limits<script_enum_value_type>::max()
};

void setup_my_enum(asbind20::engine_pointer engine)
{
    asbind20::enum_<my_enum, std::underlying_type_t<my_enum>>(engine, "my_enum")
        .value("neg_one", my_enum::neg_one)
        .value("zero", my_enum::zero)
        .value("one", my_enum::one)
        .value("huge_val", my_enum::huge_val);
}

static void test_empty_arr(asbind20::engine_pointer engine)
{
    SCOPED_TRACE(__func__);

    test_script_array::run_string(
        engine,
        "factory_primitive",
        "int[] arr;\n"
        "assert(arr.empty());\n"
        "assert(arr.size == 0);"
    );

    test_script_array::run_string(
        engine,
        "factory_enum",
        "my_enum[] arr;\n"
        "assert(arr.empty());\n"
        "assert(arr.size == 0);"
    );

    test_script_array::run_string(
        engine,
        "factory_string",
        "string[] arr;\n"
        "assert(arr.empty());\n"
        "assert(arr.size == 0);"
    );

    test_script_array::run_string(
        engine,
        "factory_script_obj",
        "script_ipair[] arr;\n"
        "assert(arr.empty());\n"
        "assert(arr.size == 0);"
    );
}

static void test_construct_arr(asbind20::engine_pointer engine)
{
    SCOPED_TRACE(__func__);

    run_string(
        engine,
        "factory_size_primitive",
        "int[] arr(n: 2);\n"
        "assert(arr.size == 2);\n"
        "assert(arr[0] == 0);\n"
        "assert(arr[1] == 0);"
    );

    run_string(
        engine,
        "factory_size_enum",
        "my_enum[] arr(n: 2);\n"
        "assert(arr.size == 2);\n"
        // FIXME: fix me
        //"assert(arr[0] == my_enum::zero);\n"
        //"assert(arr[1] == my_enum::zero);"
    );

    run_string(
        engine,
        "factory_size_string",
        "string[] arr(n: 2);\n"
        "assert(arr.size == 2);\n"
        "assert(arr[0] == \"\");\n"
        "assert(arr[1] == \"\");"
    );

    run_string(
        engine,
        "copy_factory_string",
        "string[] arr(n: 2);\n"
        "string[] arr2 = arr;\n"
        "assert(arr2.size == 2);\n"
        "assert(arr2[0] == \"\");\n"
        "assert(arr2[1] == \"\");"
    );
}
} // namespace test_script_array

TEST_F(TestArrayNative, RunScripts)
{
    auto engine = get_engine();
    asbind_test::setup_script_string(engine, true);
    test_script_array::setup_my_enum(engine);

    test_script_array::test_empty_arr(engine);
    test_script_array::test_construct_arr(engine);
}

TEST_F(TestArrayGeneric, RunScripts)
{
    auto engine = get_engine();
    asbind_test::setup_script_string(engine, true);
    test_script_array::setup_my_enum(engine);

    test_script_array::test_empty_arr(engine);
    test_script_array::test_construct_arr(engine);
}
