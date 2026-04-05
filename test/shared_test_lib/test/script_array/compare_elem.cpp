#include "test_script_array.hpp"

namespace test_script_array
{
static void check_count(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    SCOPED_TRACE(__func__);

    run_string(
        engine,
        "count_primitive",
        "int[] arr = {1, 2, 2, 2, 5};\n"
        "assert(arr.count(2) == 3);\n"
        "assert(arr.count(4) == 0);\n"
        "assert(arr.count(2, n: 3) == 2);\n"
        "assert(arr.count(2, start: 2) == 2);"
    );

    run_string(
        engine,
        "count_string",
        "string[] arr = {\"aaa\", \"abb\", \"aaa\"};\n"
        "assert(arr.count(\"aaa\") == 2);\n"
        "assert(arr.count(\"bbb\") == 0);\n"
        "assert(arr.count(\"abb\") == 1);"
    );
}

static void check_count_if(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    SCOPED_TRACE(__func__);

    run_string(
        engine,
        "count_if_primitive",
        "int[] arr = {1, 2, 3, 4, 5};\n"
        "uint c = arr.count_if(function(v) { return v > 2; });\n"
        "assert(c == 3);\n"
        "c = arr.count_if(function(v) { return v > 2; }, start: 3);\n"
        "assert(c == 2);\n"
        "c = arr.count_if(function(v) { return v > 2; }, start: -2);\n"
        "assert(c == 2);\n"
        "c = arr.count_if(function(v) { return v > 2; }, n: 2);\n"
        "assert(c == 0);"
    );

    run_string(
        engine,
        "count_if_string",
        "string[] arr = {\"aaa\", \"aab\", \"abb\", \"ccb\"};\n"
        "uint c = arr.count_if(function(v) { return v.starts_with(\"aa\"); });\n"
        "assert(c == 2);\n"
        "c = arr.count_if(function(v) { return v.ends_with(\"b\"); });\n"
        "assert(c == 3);\n"
        "c = arr.count_if(function(v) { return v.starts_with(\"b\"); });\n"
        "assert(c == 0);"
    );
}

static void check_sort(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    SCOPED_TRACE(__func__);

    run_string(
        engine,
        "sort_primitive",
        "int[] arr = {1, 3, 4, 6, 7, 9, 8, 5, 2};\n"
        "assert(arr.size == 9);\n"
        "arr.sort();\n"
        "assert(arr == {1, 2, 3, 4, 5, 6, 7, 8, 9});\n"
        "assert(arr.size == 9);\n"
        "arr.sort(0, uint(-1), false);\n"
        "assert(arr == {9, 8, 7, 6, 5, 4, 3, 2, 1});\n"
        "assert(arr.size == 9);"
    );

    run_string(
        engine,
        "sort_string",
        "string[] arr = {\"aaa\", \"ccc\", \"bbb\"};\n"
        "assert(arr.size == 3);\n"
        "arr.sort();\n"
        "assert(arr ==  {\"aaa\", \"bbb\", \"ccc\"});\n"
        "assert(arr.size == 3);\n"
        "arr.sort(asc: false);\n"
        "assert(arr ==  {\"ccc\", \"bbb\", \"aaa\"});\n"
        "assert(arr.size == 3);"
    );
}
} // namespace test_script_array

using TestArrayNative = test_script_array::basic_array_suite<false>;
using TestArrayGeneric = test_script_array::basic_array_suite<true>;

TEST_F(TestArrayNative, CompareElem)
{
    auto* engine = get_engine();
    asbind_test::setup_script_string(engine, true);

    test_script_array::check_count(engine);
    test_script_array::check_count_if(engine);
    test_script_array::check_sort(engine);
}

TEST_F(TestArrayGeneric, CompareElem)
{
    auto* engine = get_engine();
    asbind_test::setup_script_string(engine, true);

    test_script_array::check_count(engine);
    test_script_array::check_count_if(engine);
    test_script_array::check_sort(engine);
}
