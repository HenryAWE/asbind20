#include "test_script_array.hpp"

using TestArrayNative = test_script_array::basic_array_suite<false>;
using TestArrayGeneric = test_script_array::basic_array_suite<true>;

namespace test_script_array
{
static void check_resize(asbind20::engine_pointer engine)
{
    SCOPED_TRACE(__func__);

    run_string(
        engine,
        "resize_primitive",
        "array<int> arr;\n"
        "arr.resize(3);\n"
        "assert(arr == {0, 0, 0});\n"
        "arr.resize(2);\n"
        "assert(arr == {0, 0});"
    );

    run_string(
        engine,
        "resize_string",
        "array<string> arr;\n"
        "arr.resize(3);\n"
        "assert(arr == {\"\", \"\", \"\"});\n"
        "arr.resize(2);\n"
        "assert(arr == {\"\", \"\"});"
    );

    run_string(
        engine,
        "resize_script_obj_handle",
        "array<script_ipair@> arr;\n"
        "arr.resize(3);\n"
        "assert(arr == {null, null, null});\n"
        "arr.resize(2);\n"
        "assert(arr == {null, null});"
    );
}

static void check_front(asbind20::engine_pointer engine)
{
    SCOPED_TRACE(__func__);

    run_string(
        engine,
        "front_on_empty_primitive",
        "int[] arr;\n"
        "assert(arr.empty());\n"
        "arr.front = 10;\n"
        "assert(arr.front == 10);\n"
        "assert(arr.size == 1);\n"
        "arr.front = 13;\n"
        "assert(arr.front == 13);\n"
        "assert(arr.size == 1);"
    );

    run_string(
        engine,
        "front_on_empty_string",
        "string[] arr;\n"
        "assert(arr.empty());\n"
        "arr.front = \"hello\";\n"
        "assert(arr.front == \"hello\");\n"
        "assert(arr.size == 1);\n"
        "arr.front = \"world\";\n"
        "assert(arr.front == \"world\");\n"
        "assert(arr.size == 1);"
    );
}

static void check_back(asbind20::engine_pointer engine)
{
    SCOPED_TRACE(__func__);

    run_string(
        engine,
        "back_on_empty_primitive",
        "int[] arr;\n"
        "assert(arr.empty());\n"
        "arr.back = 10;\n"
        "assert(arr.back == 10);\n"
        "assert(arr.size == 1);\n"
        "arr.back = 13;\n"
        "assert(arr.back == 13);\n"
        "assert(arr.size == 1);"
    );

    run_string(
        engine,
        "back_on_empty_string",
        "string[] arr;\n"
        "assert(arr.empty());\n"
        "arr.back = \"hello\";\n"
        "assert(arr.back == \"hello\");\n"
        "assert(arr.size == 1);\n"
        "arr.back = \"world\";\n"
        "assert(arr.back == \"world\");\n"
        "assert(arr.size == 1);"
    );

    run_string(
        engine,
        "back_script_obj",
        "array<script_ipair> pairs = {script_ipair(1, 1), script_ipair(2, 2)};\n"
        "assert(pairs[0] == script_ipair(1, 1));\n"
        "pairs.front = script_ipair(-1, -2);\n"
        "assert(pairs[0] == script_ipair(-1, -2));"
    );
}

static void check_reverse(asbind20::engine_pointer engine)
{
    SCOPED_TRACE(__func__);

    run_string(
        engine,
        "reverse_primitive",
        "int[] arr = {1, 2, 3, 4, 5};\n"
        "arr.reverse(1, 3);\n"
        "assert(arr == {1, 4, 3, 2, 5});\n"
        "arr.reverse(++arr.begin(), --arr.end());\n"
        "assert(arr == {1, 2, 3, 4, 5});\n"
        "arr.reverse(arr.begin());\n"
        "assert(arr == {5, 4, 3, 2, 1});\n"
        // stop >= start, should have no effect
        "arr.reverse(--arr.end(), arr.begin());\n"
        "assert(arr == {5, 4, 3, 2, 1});"
    );

    run_string(
        engine,
        "reverse_string",
        "string[] arr = {\"aaa\", \"aab\", \"abb\"};\n"
        "arr.reverse();\n"
        "assert(arr == {\"abb\", \"aab\", \"aaa\"});"
    );
}

static void check_erase(asbind20::engine_pointer engine)
{
    SCOPED_TRACE(__func__);

    run_string(
        engine,
        "erase_primitive",
        "int[] arr = {1, 2};\n"
        "assert(arr.size == 2);\n"
        "assert(arr.begin().value == 1);\n"
        "auto it = arr.erase(arr.begin());\n"
        "assert(arr.size == 1);\n"
        "assert(it.value == 2);\n"
        "it = arr.erase(it);\n"
        "assert(arr.empty());\n"
        "assert(it == arr.end());"
    );

    run_string(
        engine,
        "erase_string",
        "string[] arr = {\"hello\", \"world\"};\n"
        "assert(arr.size == 2);\n"
        "assert(arr.begin().value == \"hello\");\n"
        "auto it = arr.erase(arr.begin());\n"
        "assert(arr.size == 1);\n"
        "assert(it.value == \"world\");\n"
        "it = arr.erase(it);\n"
        "assert(arr.empty());\n"
        "assert(it == arr.end());"
    );
}

static void check_remove(asbind20::engine_pointer engine)
{
    SCOPED_TRACE(__func__);

    run_string(
        engine,
        "remove_primitive",
        "int[] arr = {1, 2, 2, 2, 5};\n"
        "assert(arr.remove(2) == 3);\n"
        "assert(arr == {1, 5});"
    );

    run_string(
        engine,
        "remove_string",
        "string[] arr = {\"aaa\", \"abb\", \"aaa\"};\n"
        "assert(arr.remove(\"aaa\") == 2);\n"
        "assert(arr == {\"abb\"});"
    );
}

static void check_remove_if(asbind20::engine_pointer engine)
{
    SCOPED_TRACE(__func__);

    run_string(
        engine,
        "remove_if_primitive",
        "int[] arr = {1, 2, 3, 4, 5};\n"
        "arr.remove_if(function(v) { return v > 2; });\n"
        "assert(arr == {1, 2});"
    );

    run_string(
        engine,
        "remove_if_string",
        "string[] arr = {\"aaa\", \"aab\", \"abb\"};\n"
        "arr.remove_if(function(v) { return v.starts_with(\"aa\"); });\n"
        "assert(arr == {\"abb\"});"
    );
}

static void check_insert(asbind20::engine_pointer engine)
{
    SCOPED_TRACE(__func__);

    run_string(
        engine,
        "insert_primitive",
        "int[] arr = {3, 7};\n"
        "arr.insert(arr.begin(), 1);\n"
        "assert(arr == {1, 3, 7});\n"
        "arr.insert(++arr.find(3), 5);\n"
        "assert(arr == {1, 3, 5, 7});\n"
        "arr.insert(arr.end(), 9);\n"
        "assert(arr == {1, 3, 5, 7, 9});"
    );

    run_string(
        engine,
        "insert_string",
        "string[] arr = {\"B\", \"D\"};\n"
        "arr.insert(arr.begin(), \"A\");\n"
        "assert(arr == {\"A\", \"B\", \"D\"});\n"
        "arr.insert(++arr.find(\"B\"), \"C\");\n"
        "assert(arr == {\"A\", \"B\", \"C\", \"D\"});\n"
        "arr.insert(arr.end(), \"E\");\n"
        "assert(arr == {\"A\", \"B\", \"C\", \"D\", \"E\"});"
    );
}
} // namespace test_script_array

TEST_F(TestArrayNative, Resize)
{
    auto engine = get_engine();
    asbind_test::setup_script_string(engine, true);

    test_script_array::check_resize(engine);
}

TEST_F(TestArrayGeneric, Resize)
{
    auto engine = get_engine();
    asbind_test::setup_script_string(engine, true);

    test_script_array::check_resize(engine);
}

TEST_F(TestArrayNative, Properties)
{
    auto engine = get_engine();
    asbind_test::setup_script_string(engine, true);

    test_script_array::check_front(engine);
    test_script_array::check_back(engine);
}

TEST_F(TestArrayGeneric, Properties)
{
    auto engine = get_engine();
    asbind_test::setup_script_string(engine, true);

    test_script_array::check_front(engine);
    test_script_array::check_back(engine);
}

TEST_F(TestArrayNative, Reverse)
{
    auto engine = get_engine();
    asbind_test::setup_script_string(engine, true);

    test_script_array::check_reverse(engine);
}

TEST_F(TestArrayGeneric, Reverse)
{
    auto engine = get_engine();
    asbind_test::setup_script_string(engine, true);

    test_script_array::check_reverse(engine);
}

TEST_F(TestArrayNative, EraseAndRemove)
{
    auto engine = get_engine();
    asbind_test::setup_script_string(engine, true);

    test_script_array::check_erase(engine);
    test_script_array::check_remove(engine);
    test_script_array::check_remove_if(engine);
}

TEST_F(TestArrayGeneric, EraseAndRemove)
{
    auto engine = get_engine();
    asbind_test::setup_script_string(engine, true);

    test_script_array::check_erase(engine);
    test_script_array::check_remove(engine);
    test_script_array::check_remove_if(engine);
}

TEST_F(TestArrayNative, Insert)
{
    auto engine = get_engine();
    asbind_test::setup_script_string(engine, true);

    test_script_array::check_insert(engine);
}

TEST_F(TestArrayGeneric, Insert)
{
    auto engine = get_engine();
    asbind_test::setup_script_string(engine, true);

    test_script_array::check_insert(engine);
}
