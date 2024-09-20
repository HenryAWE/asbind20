#include <gtest/gtest.h>
#include "shared.hpp"
#include <asbind20/asbind.hpp>
#include <asbind20/ext/array.hpp>
#include <asbind20/ext/helper.hpp>

using namespace asbind_test;

class ext_array_suite : public asbind_test_suite
{
public:
    template <typename R = void>
    void run_string(const char* section, std::string_view code, int ret_type_id = asTYPEID_VOID)
    {
        asIScriptEngine* engine = get_engine();
        int r = 0;

        std::string func_code = asbind20::string_concat(
            engine->GetTypeDeclaration(ret_type_id, true),
            " test_ext_array(){\n",
            code,
            "\n;}"
        );

        asIScriptModule* m = engine->GetModule("test_ext_array", asGM_ALWAYS_CREATE);
        asIScriptFunction* f = nullptr;
        r = m->CompileFunction(section, func_code.c_str(), -1, 0, &f);
        ASSERT_GE(r, 0) << "Failed to compile section \"" << section << '\"';

        ASSERT_TRUE(f != nullptr);
        asIScriptContext* ctx = engine->CreateContext();
        auto result = asbind20::script_invoke<R>(ctx, f);
        ctx->Release();
        f->Release();

        ASSERT_TRUE(result_has_value(result));
        if constexpr(!std::is_void_v<R>)
            return result.value();
    }
};

TEST_F(ext_array_suite, reverse)
{
    run_string(
        "test_reverse_primitive",
        "int[] arr = {1, 2, 3, 4, 5};\n"
        "arr.reverse(1, 3);\n"
        "assert(arr == {1, 4, 3, 2, 5});"
    );

    run_string(
        "test_reverse_string",
        "string[] arr = {\"aaa\", \"aab\", \"abb\"};\n"
        "arr.reverse();\n"
        "assert(arr == {\"abb\", \"aab\", \"aaa\"});"
    );
}

TEST_F(ext_array_suite, erase_if)
{
    run_string(
        "test_erase_if_primitive",
        "int[] arr = {1, 2, 3, 4, 5};\n"
        "arr.erase_if(function(v) { return v > 2; });\n"
        "assert(arr == {1, 2});"
    );

    run_string(
        "test_erase_if_string",
        "string[] arr = {\"aaa\", \"aab\", \"abb\"};\n"
        "arr.erase_if(function(v) { return v.starts_with(\"aa\"); });\n"
        "assert(arr == {\"abb\"});"
    );
}

TEST_F(ext_array_suite, erase_value)
{
    run_string(
        "test_erase_value_primitive",
        "int[] arr = {1, 2, 2, 2, 5};\n"
        "assert(arr.erase_value(2) == 3);\n"
        "assert(arr == {1, 5});"
    );

    run_string(
        "test_erase_value_string",
        "string[] arr = {\"aaa\", \"abb\", \"aaa\"};\n"
        "assert(arr.erase_value(\"aaa\") == 2);\n"
        "assert(arr == {\"abb\"});"
    );
}

TEST_F(ext_array_suite, count_if)
{
    run_string(
        "test_count_if_primitive",
        "int[] arr = {1, 2, 3, 4, 5};\n"
        "uint c = arr.count_if(function(v) { return v > 2; });\n"
        "assert(c == 3);"
    );

    run_string(
        "test_count_if_string",
        "string[] arr = {\"aaa\", \"aab\", \"abb\"};\n"
        "uint c = arr.count_if(function(v) { return v.starts_with(\"aa\"); });\n"
        "assert(c == 2);"
    );
}

TEST_F(ext_array_suite, count)
{
    run_string(
        "test_count_primitive",
        "int[] arr = {1, 2, 2, 2, 5};\n"
        "assert(arr.count(2) == 3);"
    );

    run_string(
        "test_count_string",
        "string[] arr = {\"aaa\", \"abb\", \"aaa\"};\n"
        "assert(arr.count(\"aaa\") == 2);"
    );
}

TEST_F(asbind_test_suite, ext_array)
{
    run_file("script/test_array.as");
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
