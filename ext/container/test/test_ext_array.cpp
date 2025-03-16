#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/assert.hpp>
#include <asbind20/ext/array.hpp>
#include <asbind20/ext/stdstring.hpp>

namespace test_ext_array
{
constexpr char helper_module_name[] = "test_ext_array";

constexpr char helper_module_script[] = R"AngelScript(class my_pair
{
    int x;
    int y;

    my_pair()
    {
        x = 0;
        y = 0;
    }

    my_pair(int x, int y)
    {
        this.x = x;
        this.y = y;
    }

    bool opEquals(const my_pair&in other) const
    {
        return this.x == other.x && this.y == other.y;
    }
};
)AngelScript";

template <bool UseGeneric>
class basic_ext_array_suite : public ::testing::Test
{
public:
    void SetUp() override
    {
        using namespace asbind20;

        if constexpr(!UseGeneric)
        {
            if(has_max_portability())
                GTEST_SKIP() << "AS_MAX_PORTABILITY";
        }

        m_engine = make_script_engine();

        asbind_test::setup_message_callback(m_engine, true);
        ext::register_script_assert(
            m_engine,
            [](std::string_view msg)
            {
                auto* ctx = current_context();
                if(ctx)
                {
                    FAIL() << "array assertion failed in \"" << ctx->GetFunction()->GetScriptSectionName() << "\": " << msg;
                }
                else
                {
                    FAIL() << "array assertion failed: " << msg;
                }
            }
        );
        ext::register_script_array(m_engine, true, UseGeneric);
        m_engine->SetEngineProperty(
            AS_NAMESPACE_QUALIFIER asEP_USE_CHARACTER_LITERALS, true
        );
        ext::register_std_string(m_engine, true, UseGeneric);

        build_helper_module();
    }

    void TearDown() override
    {
        m_engine.reset();
    }

    auto get_engine() const
        -> AS_NAMESPACE_QUALIFIER asIScriptEngine*
    {
        return m_engine.get();
    }

private:
    asbind20::script_engine m_engine;

    void build_helper_module()
    {
        auto* m = m_engine->GetModule(
            helper_module_name, AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
        );
        ASSERT_NE(m, nullptr);
        m->AddScriptSection(
            "test_ext_array_helper_module",
            helper_module_script
        );
        EXPECT_GE(m->Build(), 0);
    }
};

template <typename R = void>
static void run_string(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    const char* section,
    std::string_view code,
    int ret_type_id = AS_NAMESPACE_QUALIFIER asTYPEID_VOID
)
{
    int r = 0;

    std::string func_code = asbind20::string_concat(
        engine->GetTypeDeclaration(ret_type_id, true),
        " test_ext_array(){\n",
        code,
        "\n;}"
    );

    auto* m = engine->GetModule(
        helper_module_name, AS_NAMESPACE_QUALIFIER asGM_ONLY_IF_EXISTS
    );
    AS_NAMESPACE_QUALIFIER asIScriptFunction* f = nullptr;
    r = m->CompileFunction(
        section,
        func_code.c_str(),
        -1,
        0,
        &f
    );
    ASSERT_GE(r, 0) << "Failed to compile section \"" << section << '\"';

    ASSERT_TRUE(f != nullptr);
    asbind20::request_context ctx(engine);
    auto result = asbind20::script_invoke<R>(ctx, f);
    f->Release();

    if(result.error() == AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION)
        FAIL() << "GetExceptionString: " << ctx->GetExceptionString();
    else
        ASSERT_TRUE(asbind_test::result_has_value(result));
    if constexpr(!std::is_void_v<R>)
        return result.value();
}
} // namespace test_ext_array

using ext_array_native = test_ext_array::basic_ext_array_suite<false>;
using ext_array_generic = test_ext_array::basic_ext_array_suite<true>;

namespace test_ext_array
{
void check_factory(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    run_string(
        engine,
        "test_default_factory_primitive",
        "int[] arr;\n"
        "assert(arr.empty());\n"
        "assert(arr.size == 0);"
    );

    run_string(
        engine,
        "test_default_factory_string",
        "string[] arr;\n"
        "assert(arr.empty());\n"
        "assert(arr.size == 0);"
    );

    run_string(
        engine,
        "test_factory_size_primitive",
        "int[] arr(n: 2);\n"
        "assert(arr.size == 2);\n"
        "assert(arr[0] == 0);\n"
        "assert(arr[1] == 0);"
    );

    run_string(
        engine,
        "test_factory_size_string",
        "string[] arr(n: 2);\n"
        "assert(arr.size == 2);\n"
        "assert(arr[0] == \"\");\n"
        "assert(arr[1] == \"\");"
    );

    run_string(
        engine,
        "test_factory_copy_primitive",
        "int[] arr(2, 1013);\n"
        "assert(arr.size == 2);\n"
        "assert(arr[0] == 1013);\n"
        "assert(arr[1] == 1013);"
    );

    run_string(
        engine,
        "test_factory_copy_string",
        "string[] arr(2, \"AAA\");\n"
        "assert(arr.size == 2);\n"
        "assert(arr[0] == \"AAA\");\n"
        "assert(arr[1] == \"AAA\");"
    );

    run_string(
        engine,
        "test_factory_named_arg",
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

void check_list_factory(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    run_string(
        engine,
        "test_list_factory_primitive",
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
        "test_list_factory_string",
        "string[] arr = {\"hello\", \"world\"};\n"
        "assert(!arr.empty());\n"
        "assert(arr.size == 2);\n"
        "assert(arr[0] == \"hello\");\n"
        "assert(arr[1] == \"world\");\n"
        "assert(arr.begin().value == \"hello\");"
    );

    run_string(
        engine,
        "test_list_my_pair",
        "array<my_pair> pairs = {my_pair(1, 1), my_pair(2, 2)};\n"
        "assert(pairs.size == 2);\n"
        "assert(pairs[0] == my_pair(1, 1));\n"
        "assert(pairs[1] == my_pair(2, 2));\n"
        "assert(pairs[-2] == my_pair(1, 1));\n"
        "assert(pairs[-1] == my_pair(2, 2));"
    );

    run_string(
        engine,
        "test_list_my_pair_ref",
        "my_pair p1 = my_pair();\n"
        "my_pair p2 = my_pair(1, 2);\n"
        "array<my_pair@> pairs = {p1, p2, null};\n"
        "assert(pairs.size == 3);\n"
        "assert(pairs[0] is @p1);\n"
        "assert(pairs[1] is @p2);\n"
        "assert(pairs[2] is null);"
    );
}

static void check_assignment(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    run_string(
        engine,
        "test_assignment_primitive",
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
        "test_assignment_string",
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
} // namespace test_ext_array

TEST_F(ext_array_native, factory_and_assignment)
{
    auto* engine = get_engine();
    test_ext_array::check_factory(engine);
    test_ext_array::check_list_factory(engine);
    test_ext_array::check_assignment(engine);
}

TEST_F(ext_array_generic, factory_and_assignment)
{
    auto* engine = get_engine();
    test_ext_array::check_factory(engine);
    test_ext_array::check_list_factory(engine);
    test_ext_array::check_assignment(engine);
}

namespace test_ext_array
{
void check_resize(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    run_string(
        engine,
        "test_resize_primitive",
        "array<int> arr;\n"
        "arr.resize(3);\n"
        "assert(arr == {0, 0, 0});\n"
        "arr.resize(2);\n"
        "assert(arr == {0, 0});"
    );

    run_string(
        engine,
        "test_resize_string",
        "array<string> arr;\n"
        "arr.resize(3);\n"
        "assert(arr == {\"\", \"\", \"\"});\n"
        "arr.resize(2);\n"
        "assert(arr == {\"\", \"\"});"
    );

    run_string(
        engine,
        "test_my_pair_ref_primitive",
        "array<my_pair@> arr;\n"
        "arr.resize(3);\n"
        "assert(arr == {null, null, null});\n"
        "arr.resize(2);\n"
        "assert(arr == {null, null});"
    );
}
} // namespace test_ext_array

TEST_F(ext_array_native, resize)
{
    auto* engine = get_engine();
    test_ext_array::check_resize(engine);
}

TEST_F(ext_array_generic, resize)
{
    auto* engine = get_engine();
    test_ext_array::check_resize(engine);
}

namespace test_ext_array
{
static void check_front(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    run_string(
        engine,
        "test_front_on_empty_primitive",
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
        "test_front_on_empty_string",
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

static void check_back(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    run_string(
        engine,
        "test_back_on_empty_primitive",
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
        "test_back_on_empty_string",
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
        "test_back_my_pair",
        "array<my_pair> pairs = {my_pair(1, 1), my_pair(2, 2)};\n"
        "assert(pairs[0] == my_pair(1, 1));\n"
        "pairs.front = my_pair(-1, -2);\n"
        "assert(pairs[0] == my_pair(-1, -2));"
    );
}
} // namespace test_ext_array

TEST_F(ext_array_native, properties)
{
    auto* engine = get_engine();
    test_ext_array::check_front(engine);
    test_ext_array::check_back(engine);
}

TEST_F(ext_array_generic, properties)
{
    auto* engine = get_engine();
    test_ext_array::check_front(engine);
    test_ext_array::check_back(engine);
}

namespace test_ext_array
{
void check_reverse(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    run_string(
        engine,
        "test_reverse_primitive",
        "int[] arr = {1, 2, 3, 4, 5};\n"
        "arr.reverse(1, 3);\n"
        "assert(arr == {1, 4, 3, 2, 5});\n"
        "arr.reverse(++arr.begin(), --arr.end());\n"
        "assert(arr == {1, 2, 3, 4, 5});\n"
        "arr.reverse(arr.begin());\n"
        "assert(arr == {5, 4, 3, 2, 1});\n"
        "arr.reverse(--arr.end(), arr.begin());\n" // stop >= start, should have no effect
        "assert(arr == {5, 4, 3, 2, 1});"
    );

    run_string(
        engine,
        "test_reverse_string",
        "string[] arr = {\"aaa\", \"aab\", \"abb\"};\n"
        "arr.reverse();\n"
        "assert(arr == {\"abb\", \"aab\", \"aaa\"});"
    );
}
} // namespace test_ext_array

TEST_F(ext_array_native, reverse)
{
    test_ext_array::check_reverse(get_engine());
}

TEST_F(ext_array_generic, reverse)
{
    test_ext_array::check_reverse(get_engine());
}

namespace test_ext_array
{
static void check_erase(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    run_string(
        engine,
        "test_erase_primitive",
        "int[] arr = {1, 2};\n"
        "assert(arr.size == 2);\n"
        "assert(arr.begin().value == 1);"
        "auto it = arr.erase(arr.begin());\n"
        "assert(arr.size == 1);\n"
        "assert(it.value == 2);\n"
        "it = arr.erase(it);\n"
        "assert(arr.empty());\n"
        "assert(it == arr.end());"
    );

    run_string(
        engine,
        "test_erase_string",
        "string[] arr = {\"hello\", \"world\"};\n"
        "assert(arr.size == 2);\n"
        "assert(arr.begin().value == \"hello\");"
        "auto it = arr.erase(arr.begin());\n"
        "assert(arr.size == 1);\n"
        "assert(it.value == \"world\");\n"
        "it = arr.erase(it);\n"
        "assert(arr.empty());\n"
        "assert(it == arr.end());"
    );
}

static void check_remove(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    run_string(
        engine,
        "test_remove_primitive",
        "int[] arr = {1, 2, 2, 2, 5};\n"
        "assert(arr.remove(2) == 3);\n"
        "assert(arr == {1, 5});"
    );

    run_string(
        engine,
        "test_remove_string",
        "string[] arr = {\"aaa\", \"abb\", \"aaa\"};\n"
        "assert(arr.remove(\"aaa\") == 2);\n"
        "assert(arr == {\"abb\"});"
    );
}

static void check_remove_if(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    run_string(
        engine,
        "test_remove_if_primitive",
        "int[] arr = {1, 2, 3, 4, 5};\n"
        "arr.remove_if(function(v) { return v > 2; });\n"
        "assert(arr == {1, 2});"
    );

    run_string(
        engine,
        "test_remove_if_string",
        "string[] arr = {\"aaa\", \"aab\", \"abb\"};\n"
        "arr.remove_if(function(v) { return v.starts_with(\"aa\"); });\n"
        "assert(arr == {\"abb\"});"
    );
}
} // namespace test_ext_array

TEST_F(ext_array_native, erase_and_remove)
{
    auto* engine = get_engine();
    test_ext_array::check_erase(engine);
    test_ext_array::check_remove(engine);
    test_ext_array::check_remove_if(engine);
}

TEST_F(ext_array_generic, erase_and_remove)
{
    auto* engine = get_engine();
    test_ext_array::check_erase(engine);
    test_ext_array::check_remove(engine);
    test_ext_array::check_remove_if(engine);
}

namespace test_ext_array
{
static void check_count(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    run_string(
        engine,
        "test_count_primitive",
        "int[] arr = {1, 2, 2, 2, 5};\n"
        "assert(arr.count(2) == 3);\n"
        "assert(arr.count(4) == 0);\n"
        "assert(arr.count(2, n: 3) == 2);\n"
        "assert(arr.count(2, start: 2) == 2);"
    );

    run_string(
        engine,
        "test_count_string",
        "string[] arr = {\"aaa\", \"abb\", \"aaa\"};\n"
        "assert(arr.count(\"aaa\") == 2);\n"
        "assert(arr.count(\"bbb\") == 0);\n"
        "assert(arr.count(\"abb\") == 1);"
    );
}

static void check_count_if(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    run_string(
        engine,
        "test_count_if_primitive",
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
        "test_count_if_string",
        "string[] arr = {\"aaa\", \"aab\", \"abb\", \"ccb\"};\n"
        "uint c = arr.count_if(function(v) { return v.starts_with(\"aa\"); });\n"
        "assert(c == 2);\n"
        "c = arr.count_if(function(v) { return v.ends_with(\"b\"); });\n"
        "assert(c == 3);\n"
        "c = arr.count_if(function(v) { return v.starts_with(\"b\"); });\n"
        "assert(c == 0);"
    );
}
} // namespace test_ext_array

TEST_F(ext_array_native, count)
{
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine = get_engine();
    test_ext_array::check_count(engine);
    test_ext_array::check_count_if(engine);
}

TEST_F(ext_array_generic, count)
{
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine = get_engine();
    test_ext_array::check_count(engine);
    test_ext_array::check_count_if(engine);
}

namespace test_ext_array
{
static void check_find(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    run_string(
        engine,
        "test_find_primitive",
        "int[] arr = {1, 2, 3, 4, 5};\n"
        "auto it = arr.find(2);\n"
        "assert(it.offset == 1);\n"
        "assert(@it.arr is @arr);\n"
        "assert(it.value == 2);\n"
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
        "test_find_string",
        "string[] arr = {\"aaa\", \"bbb\", \"ccc\"};\n"
        "auto it = arr.find(\"bbb\");\n"
        "assert(it.offset == 1);\n"
        "assert(@it.arr is @arr);\n"
        "assert(it.value == \"bbb\");\n"
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

static void check_contains(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    run_string(
        engine,
        "test_contains_primitive",
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
        "test_contains_string",
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
} // namespace test_ext_array

TEST_F(ext_array_native, find_and_contains)
{
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine = get_engine();
    test_ext_array::check_find(engine);
    test_ext_array::check_contains(engine);
}

TEST_F(ext_array_generic, find_and_contains)
{
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine = get_engine();
    test_ext_array::check_find(engine);
    test_ext_array::check_contains(engine);
}

namespace test_ext_array
{
static void check_insert(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    run_string(
        engine,
        "test_insert_primitive",
        "int[] arr = {3, 7};\n"
        "arr.insert(arr.begin(), 1);\n"
        "assert(arr == {1, 3 ,7});\n"
        "arr.insert(++arr.find(3), 5);\n"
        "assert(arr == {1, 3, 5, 7});\n"
        "arr.insert(arr.end(), 9);\n"
        "assert(arr == {1, 3, 5, 7, 9});"
    );

    run_string(
        engine,
        "test_insert_string",
        "string[] arr = {\"B\", \"D\"};\n"
        "arr.insert(arr.begin(), \"A\");\n"
        "assert(arr == {\"A\", \"B\", \"D\"});\n"
        "arr.insert(++arr.find(\"B\"), \"C\");\n"
        "assert(arr == {\"A\", \"B\", \"C\", \"D\"});\n"
        "arr.insert(arr.end(), \"E\");\n"
        "assert(arr == {\"A\", \"B\", \"C\", \"D\", \"E\"});"
    );
}
} // namespace test_ext_array

TEST_F(ext_array_native, insert)
{
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine = get_engine();
    test_ext_array::check_insert(engine);
}

TEST_F(ext_array_generic, insert)
{
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine = get_engine();
    test_ext_array::check_insert(engine);
}

namespace test_ext_array
{
static void check_sort(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    run_string(
        engine,
        "test_sort_primitive",
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
        "test_sort_string",
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

static void check_sort_by(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    run_string(
        engine,
        "test_sort_by_primitive",
        "int[] arr = {1, 2, 3, 4, 5, 6, 7, 8, 9};\n"
        "assert(arr.size == 9);\n"
        "arr.sort_by(function(l, r) { return l % 3 < r % 3; }, stable: true);\n"
        "assert(arr == {3, 6, 9, 1, 4, 7, 2, 5, 8});\n"
        "assert(arr.size == 9);"
        "arr.sort_by(function(l, r) { return l > r; });\n"
        "assert(arr == {9, 8, 7, 6, 5, 4, 3, 2, 1});"
    );

    run_string(
        engine,
        "test_sort_by_string",
        "string[] arr = {\"aaa\", \"ccb\", \"ccc\", \"bbb\"};\n"
        "assert(arr.size == 4);\n"
        "arr.sort_by(function(l, r) { return l[0] > r[0]; }, stable: true);\n"
        "assert(arr.size == 4);\n"
        "assert(arr == {\"ccb\", \"ccc\", \"bbb\", \"aaa\"});"
    );
}
} // namespace test_ext_array

TEST_F(ext_array_native, sort)
{
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine = get_engine();
    test_ext_array::check_sort(engine);
    test_ext_array::check_sort_by(engine);
}

TEST_F(ext_array_generic, sort)
{
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine = get_engine();
    test_ext_array::check_sort(engine);
    test_ext_array::check_sort_by(engine);
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
