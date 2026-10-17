#include "test_script_array.hpp"

using TestArrayNative = test_script_array::basic_array_suite<false>;
using TestArrayGeneric = test_script_array::basic_array_suite<true>;

namespace test_script_array
{
static void check_find(asbind20::engine_pointer engine)
{
    SCOPED_TRACE(__func__);

    run_string(
        engine,
        "find_primitive",
        "int[] arr = {1, 2, 3, 4, 5};\n"
        "auto it = arr.find(2);\n"
        "assert(it.offset == 1);\n"
        "assert(it - arr.begin() == 1);\n"
        "assert(it == arr.begin() + 1);\n"
        "assert(it == arr.end() - 4);\n"
        "assert(@it.arr is @arr);\n"
        "assert(it.value == 2);\n"
        "assert(it[-1] == 1);\n"
        "assert(it[0] == 2);\n"
        "assert(it[1] == 3);\n"
        "it = arr.find(2, start: 1);\n"
        "assert(it.offset == 1);\n"
        "it = arr.find(2, start: 2);\n"
        "assert(it == arr.end());\n"
        "it = arr.find(5);\n"
        "assert(it == --arr.end());\n"
        "it = arr.find(5, n: 2);\n"
        "assert(it == arr.end());"
    );

    run_string(
        engine,
        "find_string",
        "string[] arr = {\"aaa\", \"bbb\", \"ccc\"};\n"
        "auto it = arr.find(\"bbb\");\n"
        "assert(it.offset == 1);\n"
        "assert(it - arr.begin() == 1);\n"
        "assert(it == arr.begin() + 1);\n"
        "assert(it == arr.end() - 2);\n"
        "assert(@it.arr is @arr);\n"
        "assert(it.value == \"bbb\");\n"
        "assert(it[-1] == \"aaa\");\n"
        "assert(it[0] == \"bbb\");\n"
        "assert(it[1] == \"ccc\");\n"
        "it = arr.find(\"bbb\", start: 1);\n"
        "assert(it.offset == 1);\n"
        "it = arr.find(\"bbb\", start: 2);\n"
        "assert(it == arr.end());\n"
        "it = arr.find(\"ccc\");\n"
        "assert(it == --arr.end());\n"
        "it = arr.find(\"ccc\", n: 2);\n"
        "assert(it == arr.end());"
    );
}

static void check_contains(asbind20::engine_pointer engine)
{
    SCOPED_TRACE(__func__);

    run_string(
        engine,
        "contains_primitive",
        "int[] arr = {1, 2, 3, 4, 5};\n"
        "assert(arr.contains(2));\n"
        "assert(arr.contains(2, start: 1));\n"
        "assert(arr.contains(2, start: -4));\n"
        "assert(!arr.contains(2, start: 2));\n"
        "assert(!arr.contains(2, start: -3));\n"
        "assert(arr.contains(5));\n"
        "assert(!arr.contains(5, n: 2));"
    );

    run_string(
        engine,
        "contains_string",
        "string[] arr = {\"aaa\", \"bbb\", \"ccc\"};\n"
        "assert(arr.contains(\"bbb\"));\n"
        "assert(arr.contains(\"bbb\", start: 1));\n"
        "assert(arr.contains(\"bbb\", start: -2));\n"
        "assert(!arr.contains(\"bbb\", start: 2));\n"
        "assert(!arr.contains(\"bbb\", start: -1));\n"
        "assert(arr.contains(\"ccc\"));\n"
        "assert(!arr.contains(\"ccc\", n: 2));"
    );
}
} // namespace test_script_array

TEST_F(TestArrayNative, FindAndContains)
{
    auto engine = get_engine();
    asbind_test::setup_script_string(engine, true);

    test_script_array::check_find(engine);
    test_script_array::check_contains(engine);
}

TEST_F(TestArrayGeneric, FindAndContains)
{
    auto engine = get_engine();
    asbind_test::setup_script_string(engine, true);

    test_script_array::check_find(engine);
    test_script_array::check_contains(engine);
}
