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
        "assert(arr[2] == 2);"
    );

    run_string(
        engine,
        "test_list_factory_string",
        "string[] arr = {\"hello\", \"world\"};\n"
        "assert(!arr.empty());\n"
        "assert(arr.size == 2);\n"
        "assert(arr[0] == \"hello\");\n"
        "assert(arr[1] == \"world\");"
    );

    run_string(
        engine,
        "test_list_my_pair",
        "array<my_pair> pairs = {my_pair(1, 1), my_pair(2, 2)};\n"
        "assert(pairs.size == 2);\n"
        "assert(pairs[0] == my_pair(1, 1));\n"
        "assert(pairs[1] == my_pair(2, 2));"
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
} // namespace test_ext_array

TEST_F(ext_array_native, factory)
{
    auto* engine = get_engine();
    test_ext_array::check_factory(engine);
    test_ext_array::check_list_factory(engine);
}

TEST_F(ext_array_generic, factory)
{
    auto* engine = get_engine();
    test_ext_array::check_factory(engine);
    test_ext_array::check_list_factory(engine);
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
        "assert(arr == {1, 4, 3, 2, 5});"
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

TEST_F(ext_array_native, erase)
{
    auto* engine = get_engine();
    test_ext_array::check_remove(engine);
    test_ext_array::check_remove_if(engine);
}

TEST_F(ext_array_generic, erase)
{
    auto* engine = get_engine();
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
        "assert(arr.count(2) == 3);"
    );

    run_string(
        engine,
        "test_count_string",
        "string[] arr = {\"aaa\", \"abb\", \"aaa\"};\n"
        "assert(arr.count(\"aaa\") == 2);"
    );
}

static void check_count_if(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    run_string(
        engine,
        "test_count_if_primitive",
        "int[] arr = {1, 2, 3, 4, 5};\n"
        "uint c = arr.count_if(function(v) { return v > 2; });\n"
        "assert(c == 3);"
    );

    run_string(
        engine,
        "test_count_if_string",
        "string[] arr = {\"aaa\", \"aab\", \"abb\"};\n"
        "uint c = arr.count_if(function(v) { return v.starts_with(\"aa\"); });\n"
        "assert(c == 2);"
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

// TEST_F(asbind_test_suite, ext_array)
// {
//     if(asbind20::has_max_portability())
//         GTEST_SKIP() << "AS_MAX_PORTABILITY";

//     run_file("script/test_array.as");
// }

// TEST_F(asbind_test_suite_generic, ext_array)
// {
//     run_file("script/test_array.as");
// }

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
