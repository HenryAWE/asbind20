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

    test_script_array::test_empty_arr(engine);
}

TEST_F(TestArrayGeneric, ConstructingArray)
{
    auto engine = get_engine();
    asbind_test::setup_script_string(engine, true);
    test_script_array::setup_my_enum(engine);

    test_script_array::test_construct_arr(engine);
}
