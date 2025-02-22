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

    explicit trivial_value_class(int val)
        : value(val) {}

    trivial_value_class(int* list_buf)
    {
        value = list_buf[0] + list_buf[1];
    }

    ~trivial_value_class() = default;

    trivial_value_class& operator=(const trivial_value_class&) = default;

    bool operator==(const trivial_value_class& rhs) const
    {
        return value == rhs.value;
    }

    friend std::strong_ordering operator<=>(const trivial_value_class& lhs, const trivial_value_class& rhs)
    {
        return lhs.value <=> rhs.value;
    }

    trivial_value_class operator+(const trivial_value_class& rhs) const
    {
        int result = value + rhs.value;
        return trivial_value_class(result);
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

    void from_var_type(void* ref, int type_id)
    {
        if(type_id == asTYPEID_VOID || !asbind20::is_primitive_type(type_id))
            return;

        asbind20::visit_primitive_type(
            [this](auto* ref)
            {
                value = static_cast<int>(*ref);
            },
            type_id,
            ref
        );
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

    trivial_value_class operator-() const
    {
        return trivial_value_class(-value);
    }

    operator int() const
    {
        return value;
    }

    explicit operator bool() const
    {
        return value != 0;
    }

    int value = 0;
};

static void add_obj_last(int val, trivial_value_class* this_)
{
    this_->value += val;
}

static void mul_obj_first(trivial_value_class* this_, int val)
{
    this_->value *= val;
}

static void add_obj_last_ref(int val, trivial_value_class& this_)
{
    this_.value += val;
}

static void mul_obj_first_ref(trivial_value_class& this_, int val)
{
    this_.value *= val;
}

void register_trivial_value_class(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    value_class<trivial_value_class> c(
        engine,
        "trivial_value_class",
        AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS |
            AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_MORE_CONSTRUCTORS
    );
    c
        .behaviours_by_traits()
        .constructor<int>("int val", use_explicit)
        .list_constructor<int>("int,int")
        .opEquals()
        .opCmp()
        .opPreInc()
        .opPreDec()
        .opPostInc()
        .opPostDec()
        .opAddAssign()
        .opAdd()
        .opNeg()
        .opConv<bool>()
        .opImplConv<int>()
        .method("void set_val(int)", &trivial_value_class::set_val)
        .method(use_generic, "void set_val2(int)", fp<&trivial_value_class::set_val>)
        .method("int get_val() const", &trivial_value_class::get_val)
        .method("void add(int val)", &test_bind::add_obj_last)
        .method("void mul(int val)", &test_bind::mul_obj_first)
        .method("void add2(int val)", &test_bind::add_obj_last_ref)
        .method("void mul2(int val)", &test_bind::mul_obj_first_ref)
        .method(
            "void lambda_fun()",
            [](trivial_value_class& v)
            { v.value = 42; }
        )
        .method("void from_var_type(const ?&in)", &trivial_value_class::from_var_type)
        .method(
            "void from_int(const ?&in)",
            [](trivial_value_class& v, void* ref, int type_id)
            {
                if(type_id == AS_NAMESPACE_QUALIFIER asTYPEID_INT32)
                    v.value = *(int*)ref;
            }
        )
        .method("void add3(int val)", fp<&test_bind::add_obj_last>)
        .method(use_generic, "void mul3(int val)", fp<&test_bind::mul_obj_first_ref>) // Test mixing native and generic calling convention
        .property("int value", &trivial_value_class::value);

    EXPECT_EQ(c.get_engine(), engine);
    EXPECT_FALSE(c.force_generic());
}

void register_trivial_value_class(asbind20::use_generic_t, AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    value_class<trivial_value_class, true> c(
        engine,
        "trivial_value_class",
        AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS |
            AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_MORE_CONSTRUCTORS
    );
    c
        .behaviours_by_traits()
        .constructor<int>("int val", use_explicit)
        .list_constructor<int>("int,int")
        .opEquals()
        .opCmp()
        .opPreInc()
        .opPreDec()
        .opPostInc()
        .opPostDec()
        .opAdd()
        .opAddAssign()
        .opNeg()
        .opConv<bool>()
        .opImplConv<int>()
        .method("void set_val(int)", fp<&trivial_value_class::set_val>)
        .method(use_generic, "void set_val2(int)", fp<&trivial_value_class::set_val>)
        .method("int get_val() const", fp<&trivial_value_class::get_val>)
        .method("void add(int val)", fp<&test_bind::add_obj_last>)
        .method("void mul(int val)", fp<&test_bind::mul_obj_first>)
        .method("void add2(int val)", fp<&test_bind::add_obj_last_ref>)
        .method("void mul2(int val)", fp<&test_bind::mul_obj_first_ref>)
        .method(
            "void lambda_fun()",
            [](trivial_value_class& v)
            { v.value = 42; }
        )
        .method("void from_var_type(const ?&in)", fp<&trivial_value_class::from_var_type>, var_type<0>)
        .method(
            "void from_int(const ?&in)",
            [](trivial_value_class& v, void* ref, int type_id)
            {
                if(type_id == AS_NAMESPACE_QUALIFIER asTYPEID_INT32)
                    v.value = *(int*)ref;
            },
            var_type<0>
        )
        .method("void add3(int val)", fp<&test_bind::add_obj_last>)
        .method("void mul3(int val)", fp<&test_bind::mul_obj_first_ref>)
        .property("int value", &trivial_value_class::value);

    EXPECT_EQ(c.get_engine(), engine);
    EXPECT_TRUE(c.force_generic());
}
} // namespace test_bind

using namespace test_bind;
using namespace asbind_test;

const char* trivial_value_class_test_script = R"(
int test_0()
{
    trivial_value_class val;
    val.from_int(1013);
    assert(val.value == 1013);
    val.from_var_type(true);
    assert(val.value == 1);
    val.from_var_type(3.14);
    assert(val.value == 3);

    trivial_value_class ret;
    return ret.get_val();
}
int test_1()
{
    trivial_value_class val;
    val.set_val(42);
    assert(val.value == 42);
    assert(val == trivial_value_class(42));
    return val.get_val();
}
int test_2()
{
    trivial_value_class val;
    val.lambda_fun();
    assert(val.value == 42);
    val.set_val2(182375);
    assert(val.value < 182376);
    assert(val < trivial_value_class(182376));
    val.add(1);
    return val.get_val();
}
int test_3()
{
    trivial_value_class val;
    val.set_val(2);
    val.mul(3);
    return val.get_val();
}
int test_4()
{
    trivial_value_class val;
    val.set_val(2);
    val.add2(1);
    val.mul2(3);
    return val.get_val();
}
int test_5()
{
    trivial_value_class val(4);
    val.add3(1);
    val.mul3(2);
    val.value += 1;
    return val.value;
}
int test_6()
{
    trivial_value_class val = {2, 3};
    int result = val;
    assert(result == 5);
    assert(bool(val));
    return val.value;
}
trivial_value_class test_7()
{
    trivial_value_class val(0);
    assert(++val == trivial_value_class(1));
    trivial_value_class tmp = val++;
    assert(tmp.value == 1);
    return val;
}
trivial_value_class test_8()
{
    trivial_value_class val(2);
    assert(--val == trivial_value_class(1));
    print(to_string(val.value));
    trivial_value_class tmp = val--;
    assert(tmp.value == 1);
    return val;
}
trivial_value_class test_9()
{
    trivial_value_class val1(2);
    trivial_value_class val2(3);
    return val1 + val2;
}
trivial_value_class test_10()
{
    trivial_value_class val1(2);
    trivial_value_class val2(3);
    val1 += val2;
    assert(val2.value == 3);
    return val1;
}
trivial_value_class test_11()
{
    trivial_value_class val1(2);
    trivial_value_class val2 = -val1;
    assert(val2.value == -2);
    return val2;
}
bool test_12(trivial_value_class val)
{
    assert(val.value == 2);
    val += trivial_value_class(1);
    return val.value == 3;
}
)";

static void check_trivial_class(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
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
    check_int_result(6, 5);

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

    check_class_result(7, 2);
    check_class_result(8, 0);
    check_class_result(9, 5);
    check_class_result(10, 5);
    check_class_result(11, -2);

    auto check_bool_result = [&]<typename... Args>(int idx, Args&&... args) -> void
    {
        std::string test_name = asbind20::string_concat("test_", std::to_string(idx));
        auto test_case = asbind20::script_function<bool(Args...)>(m->GetFunctionByName(test_name.c_str()));

        asbind20::request_context ctx(engine);
        auto result = test_case(ctx, std::forward<Args>(args)...);
        ASSERT_TRUE(result_has_value(result));
        EXPECT_TRUE(*result)
            << test_name;
    };

    check_bool_result(12, trivial_value_class(2));
}

TEST_F(asbind_test_suite, trivial_value_class)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    auto* engine = get_engine();
    register_trivial_value_class(engine);

    check_trivial_class(engine);
}

TEST_F(asbind_test_suite_generic, trivial_value_class)
{
    auto* engine = get_engine();
    register_trivial_value_class(asbind20::use_generic, engine);

    check_trivial_class(engine);
}

namespace test_bind
{
class friend_ops
{
public:
    friend_ops() = default;
    friend_ops(const friend_ops&) = default;

    friend_ops(int val) noexcept
        : value(val) {}

    // Value will be size of the list.
    friend_ops(void* list_buf) noexcept
    {
        asbind20::script_init_list_repeat init_list(list_buf);
        value = static_cast<int>(init_list.size());
    }

    friend_ops& operator=(const friend_ops&) = default;

    friend bool operator==(const friend_ops& lhs, const friend_ops& rhs)
    {
        return lhs.value == rhs.value;
    }

    friend friend_ops operator-(const friend_ops& val)
    {
        return -val.value;
    }

    friend friend_ops operator+(const friend_ops& lhs, const friend_ops& rhs)
    {
        return lhs.value + rhs.value;
    }

    friend friend_ops operator-(const friend_ops& lhs, const friend_ops& rhs)
    {
        return lhs.value - rhs.value;
    }

    int value;
};

struct friend_ops_helper
{
    int predefined_value = 0;

    int by_functor_objfirst(friend_ops& this_, int additional)
    {
        this_.value += additional;
        return std::exchange(predefined_value, 0);
    }

    int by_functor_objlast(int additional, friend_ops& this_)
    {
        this_.value += additional;
        return std::exchange(predefined_value, 0);
    }

    int by_functor_objfirst_var(friend_ops& this_, int additional, void* ref, int type_id)
    {
        this_.value += additional;
        if(type_id == AS_NAMESPACE_QUALIFIER asTYPEID_INT32)
            this_.value += *(int*)ref;
        return std::exchange(predefined_value, 0);
    }

    int by_functor_objlast_var(int additional, void* ref, int type_id, friend_ops& this_)
    {
        this_.value += additional;
        if(type_id == AS_NAMESPACE_QUALIFIER asTYPEID_INT32)
            this_.value += *(int*)ref;
        return std::exchange(predefined_value, 0);
    }
};

template <bool UseGeneric>
static void register_friend_ops(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, friend_ops_helper& helper)
{
    using namespace asbind20;

    value_class<friend_ops, UseGeneric> c(
        engine,
        "friend_ops",
        asOBJ_APP_CLASS_MORE_CONSTRUCTORS | asOBJ_APP_CLASS_ALLINTS
    );

    c
        .behaviours_by_traits()
        .template constructor<int>("int")
        .list_constructor("repeat int")
        .opEquals()
        .opNeg()
        .opAdd()
        .opSub()
        .method("int by_functor_objfirst(int)", fp<&friend_ops_helper::by_functor_objfirst>, auxiliary(helper))
        .method("int by_functor_objlast(int)", fp<&friend_ops_helper::by_functor_objlast>, auxiliary(helper))
        .method("int by_functor_objfirst_var(int, const ?&in)", fp<&friend_ops_helper::by_functor_objfirst_var>, var_type<1>, auxiliary(helper))
        .method("int by_functor_objlast_var(int, const ?&in)", fp<&friend_ops_helper::by_functor_objlast_var>, var_type<1>, auxiliary(helper))
        .property("int value", offsetof(friend_ops, value));
}
} // namespace test_bind

const char* friend_ops_test_script = R"(
int test_0()
{
    friend_ops val1(2);
    friend_ops result = -val1;
    assert(result == friend_ops(-2));
    return result.value;
}
int test_1()
{
    friend_ops val1(2);
    friend_ops val2(3);
    friend_ops result = val1 + val2;
    assert(result == friend_ops(5));
    return result.value;
}
int test_2()
{
    friend_ops val1(2);
    friend_ops val2(3);
    friend_ops result = val1 - val2;
    assert(result == friend_ops(-1));
    return result.value;
}
int test_3()
{
    friend_ops arg_count = {1, 2, 3, 4, 5, 6};
    return arg_count.value;
}
int test_4()
{
    friend_ops val(1000);
    assert(val.by_functor_objfirst(13) == 182375);
    return val.value;
}
int test_5()
{
    friend_ops val(1000);
    assert(val.by_functor_objlast(13) == 182376);
    return val.value;
}
int test_6()
{
    friend_ops val(1000);
    assert(val.by_functor_objfirst_var(10, 3) == 182375);
    return val.value;
}
int test_7()
{
    friend_ops val(1000);
    assert(val.by_functor_objlast_var(10, 3) == 182376);
    return val.value;
}
)";

static void check_friend_ops(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, friend_ops_helper& helper)
{
    asIScriptModule* m = engine->GetModule("test_value_class", asGM_ALWAYS_CREATE);
    ASSERT_TRUE(m);

    m->AddScriptSection(
        "test_friend_ops.as",
        friend_ops_test_script
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

    check_int_result(0, -2);
    check_int_result(1, 5);
    check_int_result(2, -1);
    check_int_result(3, 6);

    helper.predefined_value = 182375;
    check_int_result(4, 1013);
    EXPECT_EQ(helper.predefined_value, 0);
    helper.predefined_value = 182376;
    check_int_result(5, 1013);
    EXPECT_EQ(helper.predefined_value, 0);

    helper.predefined_value = 182375;
    check_int_result(6, 1013);
    EXPECT_EQ(helper.predefined_value, 0);
    helper.predefined_value = 182376;
    check_int_result(7, 1013);
    EXPECT_EQ(helper.predefined_value, 0);
}

TEST_F(asbind_test_suite, friend_ops)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    auto* engine = get_engine();

    friend_ops_helper helper;
    register_friend_ops<false>(engine, helper);

    check_friend_ops(engine, helper);
}

TEST_F(asbind_test_suite_generic, friend_ops)
{
    auto* engine = get_engine();

    friend_ops_helper helper;
    register_friend_ops<true>(engine, helper);

    check_friend_ops(engine, helper);
}
