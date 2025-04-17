#include <gtest/gtest.h>
#include "test_ext_array.hpp"

namespace test_ext_array
{
static void check_exception_safety(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine
)
{
    run_string(
        engine,
        "ex_safety_emplace_throw",
        "array<instantly_throw> arr;\n"
        "try { arr.emplace_back(); }\n"
        "catch { assert(arr.empty()); return; }\n"
        "assert(false);"
    );

    run_string(
        engine,
        "ex_safety_throw_on_copy",
        "array<throw_on_copy> arr(2);\n"
        "assert(arr.size == 2);\n"
        "try { arr.push_back(throw_on_copy()); }\n"
        "catch { assert(arr.size == 2); return; }\n"
        "assert(false);"
    );
}
} // namespace test_ext_array

TEST_F(ext_array_native, exception_safety)
{
    test_ext_array::check_exception_safety(get_engine());
}

TEST_F(ext_array_generic, exception_safety)
{
    test_ext_array::check_exception_safety(get_engine());
}
