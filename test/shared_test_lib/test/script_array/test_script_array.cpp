#include "test_script_array.hpp"

namespace test_script_array
{
void run_string(
    asbind20::engine_pointer engine,
    asbind20::cstring_ref section,
    std::string_view code
)
{
    SCOPED_TRACE("Running section: " + std::string(section));
    run_string<void>(engine, section, code, "void");
}
} // namespace test_script_array

using TestArrayNative = test_script_array::basic_array_suite<false>;
using TestArrayGeneric = test_script_array::basic_array_suite<true>;

namespace test_script_array
{
namespace
{
    using asbind20::compat::script_enum_value_type;

    enum my_enum : script_enum_value_type
    {
        neg_one = -1,
        zero = 0,
        one = 1,
        huge_val = std::numeric_limits<script_enum_value_type>::max()
    };
} // namespace

static void setup_my_enum(asbind20::engine_pointer engine)
{
    asbind20::enum_<my_enum, std::underlying_type_t<my_enum>>(engine, "my_enum")
        .value("neg_one", my_enum::neg_one)
        .value("zero", my_enum::zero)
        .value("one", my_enum::one)
        .value("huge_val", my_enum::huge_val);
}

static void test_empty_arr(asbind20::engine_pointer engine)
{
    run_string(
        engine,
        "factory_primitive",
        "int[] arr;\n"
        "assert(arr.empty());\n"
        "assert(arr.size == 0);"
    );

    run_string(
        engine,
        "factory_enum",
        "my_enum[] arr;\n"
        "assert(arr.empty());\n"
        "assert(arr.size == 0);"
    );

    run_string(
        engine,
        "factory_string",
        "string[] arr;\n"
        "assert(arr.empty());\n"
        "assert(arr.size == 0);"
    );

    run_string(
        engine,
        "factory_script_obj",
        "script_ipair[] arr;\n"
        "assert(arr.empty());\n"
        "assert(arr.size == 0);"
    );
}

static void test_construct_arr(asbind20::engine_pointer engine)
{
    run_string(
        engine,
        "factory_size_primitive",
        "int[] arr(n: 2);\n"
        "assert(arr.size == 2);\n"
        "assert(arr[0] == 0);\n"
        "assert(arr[1] == 0);"
    );

#if ANGELSCRIPT_VERSION < 23900
    // It seems that the new enum interface of AS has some issue.
    // TODO: Wait for this upstream issue to be solved:
    // https://github.com/anjo76/angelscript/issues/84
    run_string(
        engine,
        "factory_size_enum",
        "my_enum[] arr(n: 2);\n"
        "assert(arr.size == 2);\n"
        "assert(arr[0] == my_enum::zero);\n"
        "assert(arr[1] == my_enum::zero);"
    );
#endif

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

    run_string(
        engine,
        "factory_copy_value_primitive",
        "int[] arr(2, 1013);\n"
        "assert(arr.size == 2);\n"
        "assert(arr[0] == 1013);\n"
        "assert(arr[1] == 1013);"
    );

    run_string(
        engine,
        "factory_copy_value_string",
        "string[] arr(2, \"AAA\");\n"
        "assert(arr.size == 2);\n"
        "assert(arr[0] == \"AAA\");\n"
        "assert(arr[1] == \"AAA\");"
    );

    run_string(
        engine,
        "factory_named_arg",
        "int[] int_arr(value: 1013, n: 3);\n"
        "assert(int_arr.size == 3);\n"
        "assert(int_arr[0] == 1013);\n"
        "assert(int_arr[1] == 1013);\n"
        "assert(int_arr[2] == 1013);\n"
        "string[] str_arr(value: \"AAA\", n: 2);\n"
        "assert(str_arr.size == 2);\n"
        "assert(str_arr[0] == \"AAA\");\n"
        "assert(str_arr[1] == \"AAA\");"
    );
}

static void test_list_factory(asbind20::engine_pointer engine)
{
    SCOPED_TRACE(__func__);

    run_string(
        engine,
        "list_factory_primitive",
        "int[] arr = {0, 1, 2};\n"
        "assert(!arr.empty());\n"
        "assert(arr.size == 3);\n"
        "assert(arr[0] == 0);\n"
        "assert(arr[1] == 1);\n"
        "assert(arr[2] == 2);\n"
        "assert(arr.begin().value == 0);"
    );

    run_string(
        engine,
        "list_factory_string",
        "string[] arr = {\"hello\", \"world\"};\n"
        "assert(!arr.empty());\n"
        "assert(arr.size == 2);\n"
        "assert(arr[0] == \"hello\");\n"
        "assert(arr[1] == \"world\");\n"
        "assert(arr.begin().value == \"hello\");"
    );

    run_string(
        engine,
        "list_factory_script_obj",
        "array<script_ipair> pairs = {script_ipair(1, 1), script_ipair(2, 2)};\n"
        "assert(pairs.size == 2);\n"
        "assert(pairs[0] == script_ipair(1, 1));\n"
        "assert(pairs[1] == script_ipair(2, 2));\n"
        "assert(pairs[-2] == script_ipair(1, 1));\n"
        "assert(pairs[-1] == script_ipair(2, 2));"
    );

    run_string(
        engine,
        "list_factory_script_obj_handle",
        "script_ipair p1 = script_ipair();\n"
        "script_ipair p2 = script_ipair(1, 2);\n"
        "array<script_ipair@> pairs = {p1, p2, null};\n"
        "assert(pairs.size == 3);\n"
        "assert(pairs[0] is @p1);\n"
        "assert(pairs[1] is @p2);\n"
        "assert(pairs[2] is null);"
    );
}

static void test_assignment(asbind20::engine_pointer engine)
{
    SCOPED_TRACE(__func__);

    run_string(
        engine,
        "assignment_primitive",
        "int[] arr1 = {-1, -2};\n"
        "assert(arr1[0] == -1);\n"
        "assert(arr1[1] == -2);\n"
        "int[] arr2 = {1, 2};\n"
        "arr1 = arr2;\n"
        "assert(arr1[0] == 1);\n"
        "assert(arr1[1] == 2);\n"
        "assert(@arr1 !is @arr2);"
    );

    run_string(
        engine,
        "assignment_string",
        "string[] arr1 = {\"aaa\", \"AAA\"};\n"
        "assert(arr1[0] == \"aaa\");\n"
        "assert(arr1[1] == \"AAA\");\n"
        "string[] arr2 = {\"bbb\", \"BBB\"};\n"
        "arr1 = arr2;\n"
        "assert(arr1[0] == \"bbb\");\n"
        "assert(arr1[1] == \"BBB\");\n"
        "assert(@arr1 !is @arr2);"
    );
}
} // namespace test_script_array

TEST_F(TestArrayNative, EmptyArray)
{
    auto engine = get_engine();
    asbind_test::setup_script_string(engine, true);
    test_script_array::setup_my_enum(engine);

    test_script_array::test_empty_arr(engine);
}

TEST_F(TestArrayGeneric, EmptyArray)
{
    auto engine = get_engine();
    asbind_test::setup_script_string(engine, true);
    test_script_array::setup_my_enum(engine);

    test_script_array::test_empty_arr(engine);
}

TEST_F(TestArrayNative, ConstructingArray)
{
    auto engine = get_engine();
    asbind_test::setup_script_string(engine, true);
    test_script_array::setup_my_enum(engine);

    test_script_array::test_construct_arr(engine);
}

TEST_F(TestArrayGeneric, ConstructingArray)
{
    auto engine = get_engine();
    asbind_test::setup_script_string(engine, true);
    test_script_array::setup_my_enum(engine);

    test_script_array::test_construct_arr(engine);
}

TEST_F(TestArrayNative, ListFactory)
{
    auto engine = get_engine();
    asbind_test::setup_script_string(engine, true);

    test_script_array::test_list_factory(engine);
}

TEST_F(TestArrayGeneric, ListFactory)
{
    auto engine = get_engine();
    asbind_test::setup_script_string(engine, true);

    test_script_array::test_list_factory(engine);
}

TEST_F(TestArrayNative, Assignment)
{
    auto engine = get_engine();
    asbind_test::setup_script_string(engine, true);

    test_script_array::test_assignment(engine);
}

TEST_F(TestArrayGeneric, Assignment)
{
    auto engine = get_engine();
    asbind_test::setup_script_string(engine, true);

    test_script_array::test_assignment(engine);
}
