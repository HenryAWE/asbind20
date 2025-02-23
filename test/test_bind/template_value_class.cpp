#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>

namespace test_bind
{
class template_val
{
public:
    template_val() = delete;

    template_val(AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
        : subtype_id(ti->GetSubTypeId()) {}

    template_val(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, const template_val& val)
        : subtype_id(ti->GetSubTypeId()), value(val.value) {}

    template_val(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, int val)
        : subtype_id(ti->GetSubTypeId()), value(val) {}

    template_val(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, asbind20::script_init_list_repeat list)
        : subtype_id(ti->GetSubTypeId()), value(0)
    {
        int* start = static_cast<int*>(list.data());
        int* sentinel = start + list.size();

        for(int* it = start; it != sentinel; ++it)
            value += *it;
    }

    ~template_val() = default;

    int subtype_id;
    int value = 0;
};

class template_val_complex
{
public:
    template_val_complex() = delete;

    template_val_complex(AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
        : subtype_id(ti->GetSubTypeId()) {}

    template_val_complex(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, const template_val_complex& val)
        : subtype_id(ti->GetSubTypeId()), str(val.str) {}

    template_val_complex(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, int val)
        : subtype_id(ti->GetSubTypeId()), str(std::to_string(val)) {}

    // Pattern: {repeat int}
    template_val_complex(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, void* list_buf)
        : subtype_id(ti->GetSubTypeId())
    {
        asbind20::script_init_list_repeat list(list_buf);
        int* start = static_cast<int*>(list.data());
        int* sentinel = start + list.size();

        for(int* it = start; it != sentinel; ++it)
        {
            if(it != start)
                str += ", ";
            str += std::to_string(*it);
        }
    }

    ~template_val_complex() = default;

    int subtype_id;
    std::string str;
};

// Template specialization for AngelScript
// template_val_complex<int>
class template_val_complex_spec
{
public:
    template_val_complex_spec() = default;

    template_val_complex_spec(const template_val_complex& val)
        : str(val.str) {}

    template_val_complex_spec(int val)
        : str(std::to_string(val)) {}

    // Pattern: {repeat int}
    template_val_complex_spec(void* list_buf)
    {
        asbind20::script_init_list_repeat list(list_buf);
        int* start = static_cast<int*>(list.data());
        int* sentinel = start + list.size();

        str += "SPEC: ";
        for(int* it = start; it != sentinel; ++it)
        {
            if(it != start)
                str += ", ";
            str += std::to_string(*it);
        }
    }

    ~template_val_complex_spec() = default;

    std::string str;
};

static bool template_callback(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, bool& no_gc)
{
    no_gc = true;

    int subtype_id = ti->GetSubTypeId();
    return subtype_id == AS_NAMESPACE_QUALIFIER asTYPEID_INT32 ||
           subtype_id == AS_NAMESPACE_QUALIFIER asTYPEID_FLOAT;
}

static void create_template_val(void* mem, AS_NAMESPACE_QUALIFIER asITypeInfo* ti, int val)
{
    new(mem) template_val(ti, val);
}

static void register_template_val_class(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    auto wrapper = [](AS_NAMESPACE_QUALIFIER asITypeInfo* ti, int val1, int val2, void* mem) -> void
    {
        new(mem) template_val(ti, val1 * 100 + val2);
    };

    constexpr AS_NAMESPACE_QUALIFIER asQWORD flags =
        AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_CDA |
        AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS |
        AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_MORE_CONSTRUCTORS;
    template_value_class<template_val>(
        engine,
        "template_val<T>",
        flags
    )
        .template_callback(template_callback)
        .default_constructor()
        .opAssign()
        .constructor_function("int", use_explicit, &create_template_val)
        .constructor_function("int,int", wrapper)
        .list_constructor<int, policies::repeat_list_proxy>("repeat int")
        .destructor()
        .property("int subtype_id", &template_val::subtype_id)
        .property("int value", &template_val::value);

    template_value_class<template_val_complex>(
        engine,
        "template_val_complex<T>",
        flags | AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_COPY_CONSTRUCTOR
    )
        .template_callback(template_callback)
        .default_constructor()
        .copy_constructor()
        .opAssign()
        .constructor<int>("int", use_explicit)
        .list_constructor("repeat int")
        .destructor()
        .property("int subtype_id", &template_val_complex::subtype_id)
        .property("string str", &template_val_complex::str);

    value_class<template_val_complex_spec>(
        engine,
        "template_val_complex<int>",
        AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS | AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_MORE_CONSTRUCTORS
    )
        .default_constructor()
        .copy_constructor()
        .opAssign()
        .constructor<int>("int", use_explicit)
        .list_constructor("repeat int")
        .destructor()
        .method(
            "int get_subtype_id() const property",
            [](const template_val_complex_spec&) -> int
            {
                return AS_NAMESPACE_QUALIFIER asTYPEID_INT32;
            }
        )
        .property("string str", &template_val_complex_spec::str);
}

static void register_template_val_class(asbind20::use_generic_t, AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    auto wrapper = [](AS_NAMESPACE_QUALIFIER asITypeInfo* ti, int val1, int val2, void* mem) -> void
    {
        new(mem) template_val(ti, val1 * 100 + val2);
    };

    constexpr AS_NAMESPACE_QUALIFIER asQWORD flags =
        AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_CDA |
        AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS |
        AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_MORE_CONSTRUCTORS;
    template_value_class<template_val, true>(
        engine,
        "template_val<T>",
        flags
    )
        .template_callback(fp<&template_callback>)
        .default_constructor()
        .constructor_function("int", use_explicit, fp<&create_template_val>)
        .constructor_function("int,int", wrapper)
        .list_constructor<int, policies::repeat_list_proxy>("repeat int")
        .destructor()
        .property("int subtype_id", &template_val::subtype_id)
        .property("int value", &template_val::value);

    template_value_class<template_val_complex, true>(
        engine,
        "template_val_complex<T>",
        flags | AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_COPY_CONSTRUCTOR
    )
        .template_callback(fp<&template_callback>)
        .default_constructor()
        .copy_constructor()
        .opAssign()
        .constructor<int>("int", use_explicit)
        .list_constructor("repeat int")
        .destructor()
        .property("int subtype_id", &template_val_complex::subtype_id)
        .property("string str", &template_val_complex::str);

    value_class<template_val_complex_spec, true>(
        engine,
        "template_val_complex<int>",
        AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS | AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_MORE_CONSTRUCTORS
    )
        .behaviours_by_traits()
        .constructor<int>("int", use_explicit)
        .list_constructor("repeat int")
        .method(
            "int get_subtype_id() const property",
            [](const template_val_complex_spec&) -> int
            {
                return AS_NAMESPACE_QUALIFIER asTYPEID_INT32;
            }
        )
        .property("string str", &template_val_complex_spec::str);
}
} // namespace test_bind

using asbind_test::asbind_test_suite;
using asbind_test::asbind_test_suite_generic;

const char* template_value_class_test_script = R"(
int test_0()
{
    template_val<int> val;
    return val.subtype_id;
}
int test_1()
{
    template_val<float> val;
    return val.subtype_id;
}
int test_2()
{
    template_val<int> val(42);
    assert(val.value == 42);
    return val.subtype_id;
}
int test_3()
{
    template_val<float> val(10, 13);
    assert(val.value == 1013);
    return val.subtype_id;
}
int test_4()
{
    template_val<int> val = {1000, 10, 3};
    assert(val.value == 1013);
    return val.subtype_id;
}
int test_5()
{
    template_val_complex<float> val = {1, 2, 3};
    assert(val.str == "1, 2, 3");
    return val.subtype_id;
}
int test_6()
{
    template_val_complex<int> val = {1, 2, 3};
    assert(val.str == "SPEC: 1, 2, 3");
    return val.subtype_id;
})";

static void check_template_val_class(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = engine->GetModule("test_template_value_class", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE);
    ASSERT_TRUE(m);

    m->AddScriptSection(
        "test_template_value_class.as",
        template_value_class_test_script
    );
    ASSERT_GE(m->Build(), 0);

    auto check_int_result = [&](int idx, int expected_val) -> void
    {
        std::string test_name = asbind20::string_concat("test_", std::to_string(idx));
        auto test_case = asbind20::script_function<int()>(m->GetFunctionByName(test_name.c_str()));

        asbind20::request_context ctx(engine);
        auto result = test_case(ctx);

        using asbind_test::result_has_value;
        ASSERT_TRUE(result_has_value(result));
        EXPECT_EQ(*result, expected_val)
            << test_name;
    };

    check_int_result(0, AS_NAMESPACE_QUALIFIER asTYPEID_INT32);
    check_int_result(1, AS_NAMESPACE_QUALIFIER asTYPEID_FLOAT);
    check_int_result(2, AS_NAMESPACE_QUALIFIER asTYPEID_INT32);
    check_int_result(3, AS_NAMESPACE_QUALIFIER asTYPEID_FLOAT);
    check_int_result(4, AS_NAMESPACE_QUALIFIER asTYPEID_INT32);
    check_int_result(5, AS_NAMESPACE_QUALIFIER asTYPEID_FLOAT);
    check_int_result(6, AS_NAMESPACE_QUALIFIER asTYPEID_INT32);
}

TEST_F(asbind_test_suite, template_val_class)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    auto* engine = get_engine();

    test_bind::register_template_val_class(engine);
    check_template_val_class(engine);
}

TEST_F(asbind_test_suite_generic, template_val_class)
{
    auto* engine = get_engine();

    test_bind::register_template_val_class(asbind20::use_generic, engine);
    check_template_val_class(engine);
}
