#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>

namespace test_bind
{
class trivial_value_class
{
public:
    trivial_value_class() = default;
    trivial_value_class(const trivial_value_class&) = default;

    trivial_value_class(int val)
        : value(val) {}

    ~trivial_value_class() = default;

    trivial_value_class& operator=(const trivial_value_class&) = default;

    friend bool operator==(const trivial_value_class& lhs, const trivial_value_class& rhs)
    {
        return lhs.value == rhs.value;
    }

    friend std::strong_ordering operator<=>(const trivial_value_class& lhs, const trivial_value_class& rhs)
    {
        return lhs.value <=> rhs.value;
    }

    trivial_value_class operator+(const trivial_value_class& rhs) const
    {
        int result = value + rhs.value;
        return result;
    }

    trivial_value_class& operator+=(const trivial_value_class& rhs)
    {
        value += rhs.value;
        return *this;
    }

    int get_val() const
    {
        return value;
    }

    void set_val(int new_val)
    {
        value = new_val;
    }

    trivial_value_class& operator++()
    {
        ++value;
        return *this;
    }

    trivial_value_class operator++(int)
    {
        trivial_value_class tmp(*this);
        ++*this;
        return tmp;
    }

    trivial_value_class& operator--()
    {
        --value;
        return *this;
    }

    trivial_value_class operator--(int)
    {
        trivial_value_class tmp(*this);
        --*this;
        return tmp;
    }

    int value = 0;
};

void add_obj_last(int val, trivial_value_class* this_)
{
    this_->value += val;
}

void mul_obj_first(trivial_value_class* this_, int val)
{
    this_->value *= val;
}

void add_obj_last_ref(int val, trivial_value_class& this_)
{
    this_.value += val;
}

void mul_obj_first_ref(trivial_value_class& this_, int val)
{
    this_.value *= val;
}

void register_trivial_value_class(asIScriptEngine* engine)
{
    using namespace asbind20;

    value_class<trivial_value_class> c(
        engine,
        "trivia_val_class",
        asOBJ_APP_CLASS_CDAK | asOBJ_APP_CLASS_ALLINTS | asOBJ_APP_CLASS_MORE_CONSTRUCTORS
    );
    c
        .common_behaviours()
        .template constructor<int>("void f(int val)")
        .opEquals()
        .opCmp()
        .opPreInc()
        .opPreDec()
        .opPostInc()
        .opPostDec()
        .opAddAssign()
        .opAdd(use_generic) // TODO: Check why this wrapper has bug in native mode for some classes
        .method("void set_val(int)", &trivial_value_class::set_val)
        .method<&trivial_value_class::set_val>(use_generic, "void set_val2(int)")
        .method("int get_val() const", &trivial_value_class::get_val)
        .method("void add(int val)", &test_bind::add_obj_last)
        .method("void mul(int val)", &test_bind::mul_obj_first)
        .method("void add2(int val)", &test_bind::add_obj_last_ref)
        .method("void mul2(int val)", &test_bind::mul_obj_first_ref)
        .method<&test_bind::add_obj_last>("void add3(int val)")
        .method<&test_bind::mul_obj_first_ref>(use_generic, "void mul3(int val)"); // Test mixing native and generic calling convention
    c
        .property("int value", &trivial_value_class::value);
}

void register_trivial_value_class(asbind20::use_generic_t, asIScriptEngine* engine)
{
    using namespace asbind20;

    value_class<trivial_value_class, true> c(
        engine,
        "trivia_val_class",
        asOBJ_APP_CLASS_CDAK | asOBJ_APP_CLASS_ALLINTS | asOBJ_APP_CLASS_MORE_CONSTRUCTORS
    );
    c
        .common_behaviours()
        .template constructor<int>("void f(int val)")
        .opEquals()
        .opCmp()
        .opPreInc()
        .opPreDec()
        .opPostInc()
        .opPostDec()
        .opAdd()
        .opAddAssign()
        .method<&trivial_value_class::set_val>("void set_val(int)")
        .method<&trivial_value_class::set_val>(use_generic, "void set_val2(int)")
        .method<&trivial_value_class::get_val>("int get_val() const")
        .method<&test_bind::add_obj_last>("void add(int val)")
        .method<&test_bind::mul_obj_first>("void mul(int val)")
        .method<&test_bind::add_obj_last_ref>("void add2(int val)")
        .method<&test_bind::mul_obj_first_ref>("void mul2(int val)")
        .method<&test_bind::add_obj_last>("void add3(int val)")
        .method<&test_bind::mul_obj_first_ref>("void mul3(int val)");
    c
        .property("int value", &trivial_value_class::value);
}
} // namespace test_bind

using namespace test_bind;
using namespace asbind_test;

const char* trivial_value_class_test_script = R"(
int test_0()
{
    trivia_val_class val;
    return val.get_val();
}
int test_1()
{
    trivia_val_class val;
    val.set_val(42);
    assert(val.value == 42);
    assert(val == trivia_val_class(42));
    return val.get_val();
}
int test_2()
{
    trivia_val_class val;
    val.set_val2(182375);
    assert(val.value < 182376);
    assert(val < trivia_val_class(182376));
    val.add(1);
    return val.get_val();
}
int test_3()
{
    trivia_val_class val;
    val.set_val(2);
    val.mul(3);
    return val.get_val();
}
int test_4()
{
    trivia_val_class val;
    val.set_val(2);
    val.add2(1);
    val.mul2(3);
    return val.get_val();
}
int test_5()
{
    trivia_val_class val(4);
    val.add3(1);
    val.mul3(2);
    val.value += 1;
    return val.value;
}
trivia_val_class test_6()
{
    trivia_val_class val(0);
    assert(++val == trivia_val_class(1));
    trivia_val_class tmp = val++;
    assert(tmp.value == 1);
    return val;
}
trivia_val_class test_7()
{
    trivia_val_class val(2);
    assert(--val == trivia_val_class(1));
    print(to_string(val.value));
    trivia_val_class tmp = val--;
    assert(tmp.value == 1);
    return val;
}
trivia_val_class test_8()
{
    trivia_val_class val1(2);
    trivia_val_class val2(3);
    return val1 + val2;
}
trivia_val_class test_9()
{
    trivia_val_class val1(2);
    trivia_val_class val2(3);
    val1 += val2;
    assert(val2.value == 3);
    return val1;
}
)";

static void check_trivial_class(asIScriptEngine* engine)
{
    asIScriptModule* m = engine->GetModule("test_value_class", asGM_ALWAYS_CREATE);
    ASSERT_TRUE(m);

    m->AddScriptSection(
        "test_trivial_value_class.as",
        trivial_value_class_test_script
    );
    ASSERT_GE(m->Build(), 0);

    auto check_int_result = [&](int idx, int expected_val) -> void
    {
        std::string test_name = asbind20::string_concat("test_", std::to_string(idx));
        auto test_case = asbind20::script_function<int()>(m->GetFunctionByName(test_name.c_str()));

        asbind20::request_context ctx(engine);
        auto result = test_case(ctx);
        ASSERT_TRUE(result_has_value(result));
        EXPECT_EQ(*result, expected_val)
            << test_name;
    };

    check_int_result(0, 0);
    check_int_result(1, 42);
    check_int_result(2, 182376);
    check_int_result(3, 6);
    check_int_result(4, 9);
    check_int_result(5, 11);

    auto check_class_result = [&](int idx, int expected_val) -> void
    {
        std::string test_name = asbind20::string_concat("test_", std::to_string(idx));
        auto test_case = asbind20::script_function<trivial_value_class()>(m->GetFunctionByName(test_name.c_str()));

        asbind20::request_context ctx(engine);
        auto result = test_case(ctx);
        ASSERT_TRUE(result_has_value(result));
        EXPECT_EQ(result->get_val(), expected_val)
            << test_name;
    };

    check_class_result(6, 2);
    check_class_result(7, 0);
    check_class_result(8, 5);
    check_class_result(9, 5);
}

TEST_F(asbind_test_suite, trivial_value_class)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    asIScriptEngine* engine = get_engine();
    register_trivial_value_class(engine);

    check_trivial_class(engine);
}

TEST_F(asbind_test_suite_generic, trivial_value_class)
{
    asIScriptEngine* engine = get_engine();
    register_trivial_value_class(asbind20::use_generic, engine);

    check_trivial_class(engine);
}
