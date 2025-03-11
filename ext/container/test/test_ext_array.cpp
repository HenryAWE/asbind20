#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/assert.hpp>
#include <asbind20/ext/array.hpp>
#include <asbind20/ext/stdstring.hpp>

namespace test_ext_array
{
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
                FAIL() << "array assertion failed: " << msg;
            }
        );
        ext::register_script_array(m_engine, true, UseGeneric);
        ext::register_std_string(m_engine, true, UseGeneric);
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
        "test_ext_array", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
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
    GTEST_SKIP();
    test_ext_array::check_reverse(get_engine());
}

TEST_F(ext_array_generic, reverse)
{
    GTEST_SKIP();
    test_ext_array::check_reverse(get_engine());
}

namespace test_ext_array
{
static void check_erase_value(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    run_string(
        engine,
        "test_erase_value_primitive",
        "int[] arr = {1, 2, 2, 2, 5};\n"
        "assert(arr.erase_value(2) == 3);\n"
        "assert(arr == {1, 5});"
    );

    run_string(
        engine,
        "test_erase_value_string",
        "string[] arr = {\"aaa\", \"abb\", \"aaa\"};\n"
        "assert(arr.erase_value(\"aaa\") == 2);\n"
        "assert(arr == {\"abb\"});"
    );
}

static void check_erase_if(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    run_string(
        engine,
        "test_erase_if_primitive",
        "int[] arr = {1, 2, 3, 4, 5};\n"
        "arr.erase_if(function(v) { return v > 2; });\n"
        "assert(arr == {1, 2});"
    );

    run_string(
        engine,
        "test_erase_if_string",
        "string[] arr = {\"aaa\", \"aab\", \"abb\"};\n"
        "arr.erase_if(function(v) { return v.starts_with(\"aa\"); });\n"
        "assert(arr == {\"abb\"});"
    );
}
} // namespace test_ext_array

TEST_F(ext_array_native, erase)
{
    GTEST_SKIP();
    auto* engine = get_engine();
    test_ext_array::check_erase_value(engine);
    test_ext_array::check_erase_if(engine);
}

TEST_F(ext_array_generic, erase)
{
    GTEST_SKIP();
    auto* engine = get_engine();
    test_ext_array::check_erase_value(engine);
    test_ext_array::check_erase_if(engine);
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

void check_count_if(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
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
    // test_ext_array::check_count_if(engine);
}

TEST_F(ext_array_generic, count)
{
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine = get_engine();
    test_ext_array::check_count(engine);
    //  test_ext_array::check_count_if(engine);
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
