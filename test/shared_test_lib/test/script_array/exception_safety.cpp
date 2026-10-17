#include "test_script_array.hpp"

#ifndef ASBIND20_NO_EXCEPTIONS

using TestArrayNative = test_script_array::basic_array_suite<false>;
using TestArrayGeneric = test_script_array::basic_array_suite<true>;

namespace test_script_array
{
static void check_exception_safety(asbind20::engine_pointer engine)
{
    SCOPED_TRACE(__func__);

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

    run_string(
        engine,
        "ex_safety_throw_on_copy_ilist",
        "try { array<throw_on_copy> arr = { throw_on_copy(), throw_on_copy() }; }\n"
        "catch { return; }\n"
        "assert(false);"
    );
}
} // namespace test_script_array

TEST_F(TestArrayNative, ExceptionSafety)
{
    // AngelScript 2.38's x64 native calling convention trampoline cannot
    // unwind the C++ exception thrown by the assignment operator in the
    // initialization list case when AddressSanitizer is enabled, so the
    // exception is not translated into a script exception.
    ASBIND_TEST_SKIP_IF_ADDRESS_SANITIZER();

    test_script_array::check_exception_safety(get_engine());
}

TEST_F(TestArrayGeneric, ExceptionSafety)
{
    test_script_array::check_exception_safety(get_engine());
}

#endif // !ASBIND20_NO_EXCEPTIONS
