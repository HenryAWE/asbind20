#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/assert.hpp>

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

    trivial_value_class operator%(const trivial_value_class& rhs) const
    {
        int result = value % rhs.value;
        return trivial_value_class(result);
    }

    trivial_value_class& operator%=(const trivial_value_class& rhs)
    {
        value %= rhs.value;
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
        if(asbind20::is_void_type(type_id) || !asbind20::is_primitive_type(type_id))
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

constexpr AS_NAMESPACE_QUALIFIER asQWORD trivial_value_class_flags =
    AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_MORE_CONSTRUCTORS |
    AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS;

void register_trivial_value_class(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    value_class<trivial_value_class> c(
        engine,
        "trivial_value_class",
        trivial_value_class_flags
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
        .opModAssign()
        .opAdd()
        .opNeg()
        .opMod()
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
        trivial_value_class_flags
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
        .opMod()
        .opModAssign()
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
bool test_13(trivial_value_class val)
{
    assert(val.value == 4);
    val %= trivial_value_class(3);
    return val.value == 1;
}
bool test_14(trivial_value_class val)
{
    assert(val.value == 4);
    trivial_value_class result = val % trivial_value_class(3);
    return result.value == 1;
}
)";

static void check_trivial_class(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = engine->GetModule(
        "test_value_class", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
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
        ASSERT_TRUE(asbind_test::result_has_value(result));
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
        ASSERT_TRUE(asbind_test::result_has_value(result));
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
        ASSERT_TRUE(asbind_test::result_has_value(result));
        EXPECT_TRUE(*result)
            << test_name;
    };

    check_bool_result(12, trivial_value_class(2));
    check_bool_result(13, trivial_value_class(4));
    check_bool_result(14, trivial_value_class(4));
}

template <bool UseGeneric>
class basic_trivial_value_class_suite : public ::testing::Test
{
public:
    void SetUp() override
    {
        if constexpr(!UseGeneric)
        {
            if(asbind20::has_max_portability())
                GTEST_SKIP() << "AS_MAX_PORTABILITY";
        }

        m_engine = asbind20::make_script_engine();
        asbind_test::setup_message_callback(m_engine, true);
        asbind20::ext::register_script_assert(
            m_engine,
            [](std::string_view msg)
            {
                FAIL() << "trivial_value_class assertion failed: " << msg;
            }
        );
        if constexpr(UseGeneric)
            register_trivial_value_class(asbind20::use_generic, m_engine);
        else
            register_trivial_value_class(m_engine);
    }

    void TearDown() override
    {
        m_engine.reset();
    }

    auto get_engine() const noexcept
        -> AS_NAMESPACE_QUALIFIER asIScriptEngine*
    {
        return m_engine.get();
    }

private:
    asbind20::script_engine m_engine;
};
} // namespace test_bind

using trivial_value_class_native = test_bind::basic_trivial_value_class_suite<false>;
using trivial_value_class_generic = test_bind::basic_trivial_value_class_suite<true>;

TEST_F(trivial_value_class_native, check_trivial_class)
{
    auto* engine = get_engine();
    test_bind::check_trivial_class(engine);
}

TEST_F(trivial_value_class_generic, check_trivial_class)
{
    auto* engine = get_engine();
    test_bind::check_trivial_class(engine);
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
        AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_MORE_CONSTRUCTORS | AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS
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
    auto* m = engine->GetModule(
        "test_value_class", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
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
        ASSERT_TRUE(asbind_test::result_has_value(result));
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

template <bool UseGeneric>
class basic_friend_ops_suite : public ::testing::Test
{
public:
    void SetUp() override
    {
        if constexpr(!UseGeneric)
        {
            if(asbind20::has_max_portability())
                GTEST_SKIP() << "AS_MAX_PORTABILITY";
        }

        m_engine = asbind20::make_script_engine();
        m_helper = friend_ops_helper{};

        asbind_test::setup_message_callback(m_engine, true);
        asbind20::ext::register_script_assert(
            m_engine,
            [](std::string_view msg)
            {
                FAIL() << "friend_ops assertion failed: " << msg;
            }
        );
        register_friend_ops<UseGeneric>(m_engine, m_helper);
    }

    void TearDown() override
    {
        m_engine.reset();
    }

    auto get_engine() const noexcept
        -> AS_NAMESPACE_QUALIFIER asIScriptEngine*
    {
        return m_engine.get();
    }

    friend_ops_helper& get_helper()
    {
        return m_helper;
    }

private:
    asbind20::script_engine m_engine;
    friend_ops_helper m_helper;
};
} // namespace test_bind

using friend_ops_native = test_bind::basic_friend_ops_suite<false>;
using friend_ops_generic = test_bind::basic_friend_ops_suite<true>;

TEST_F(friend_ops_native, check_friend_ops)
{
    auto* engine = get_engine();
    check_friend_ops(engine, get_helper());
}

TEST_F(friend_ops_generic, check_friend_ops)
{
    auto* engine = get_engine();
    check_friend_ops(engine, get_helper());
}

namespace test_bind
{
struct comp_data
{
    int comp_a;
    int comp_b;
};

class base_val_class
{
public:
    base_val_class()
        : a(0), indirect(new comp_data{1, 2}), b(3) {}

    base_val_class(const base_val_class& other)
        : a(other.a), indirect(new comp_data(*other.indirect)), b(3) {}

    ~base_val_class()
    {
        delete indirect;
    }

    base_val_class& operator=(const base_val_class& rhs)
    {
        if(this == &rhs)
            return *this;

        a = rhs.a;
        *indirect = *rhs.indirect;
        b = rhs.b;

        return *this;
    }

    int a;
    comp_data* const indirect;
    int b;
};

template <bool UseGeneric, bool UseMP, bool CompUseMP>
static void register_base_val_class(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using asbind20::composite;

    asbind20::value_class<base_val_class, UseGeneric> c(engine, "base_val_class");
    c
        .behaviours_by_traits()
        .property("int a", &base_val_class::a)
        .property("int b", &base_val_class::b);

    if constexpr(UseMP && CompUseMP)
    {
        c
            .property("int comp_a", &comp_data::comp_a, asbind20::composite(&base_val_class::indirect))
            .property("int comp_b", &comp_data::comp_b, asbind20::composite(&base_val_class::indirect));
    }
    else if constexpr(UseMP)
    {
        c
            .property("int comp_a", &comp_data::comp_a, composite(offsetof(base_val_class, indirect)))
            .property("int comp_b", &comp_data::comp_b, composite(offsetof(base_val_class, indirect)));
    }
    else if constexpr(CompUseMP)
    {
        c
            .property("int comp_a", offsetof(comp_data, comp_a), composite(&base_val_class::indirect))
            .property("int comp_b", offsetof(comp_data, comp_b), composite(&base_val_class::indirect));
    }
    else // Both are represented by offset
    {
        c
            .property("int comp_a", offsetof(comp_data, comp_a), composite(offsetof(base_val_class, indirect)))
            .property("int comp_b", offsetof(comp_data, comp_b), composite(offsetof(base_val_class, indirect)));
    }
}

static void check_val_class_comp_property(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = engine->GetModule(
        "val_class_comp_prop", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    ASSERT_TRUE(m);

    m->AddScriptSection(
        "test_comp_prop",
        "base_val_class create_val() { return base_val_class(); }\n"
        "void test()\n"
        "{\n"
        "    base_val_class c;\n"
        "    assert(c.a == 0);\n"
        "    assert(c.comp_a == 1);\n"
        "    assert(c.comp_b == 2);\n"
        "    assert(c.b == 3);\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    {
        auto* f = m->GetFunctionByName("create_val");

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<base_val_class>(ctx, f);
        ASSERT_TRUE(asbind_test::result_has_value(result));
        EXPECT_EQ(result.value().a, 0);
        EXPECT_EQ(result.value().indirect->comp_a, 1);
        EXPECT_EQ(result.value().indirect->comp_b, 2);
        EXPECT_EQ(result.value().b, 3);
    }

    {
        auto* f = m->GetFunctionByName("test");

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<void>(ctx, f);
        EXPECT_TRUE(asbind_test::result_has_value(result));
    }
}

template <bool UseGeneric, bool UseMP, bool CompUseMP>
static void setup_val_class_comp_prop_test(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    ext::register_script_assert(
        engine,
        [](std::string_view msg)
        {
            FAIL() << "val_class_comp_prop failed: " << msg;
        }
    );

    register_base_val_class<UseGeneric, UseMP, CompUseMP>(engine);
}
} // namespace test_bind

TEST(val_class_comp_prop, native_off_off)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    using namespace test_bind;
    auto engine = asbind20::make_script_engine();
    setup_val_class_comp_prop_test<false, false, false>(engine);
    check_val_class_comp_property(engine);
}

TEST(val_class_comp_prop, generic_off_off)
{
    using namespace test_bind;
    auto engine = asbind20::make_script_engine();
    setup_val_class_comp_prop_test<true, false, false>(engine);
    check_val_class_comp_property(engine);
}

TEST(val_class_comp_prop, native_mp_off)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    using namespace test_bind;
    auto engine = asbind20::make_script_engine();
    setup_val_class_comp_prop_test<false, true, false>(engine);
    check_val_class_comp_property(engine);
}

TEST(val_class_comp_prop, generic_mp_off)
{
    using namespace test_bind;
    auto engine = asbind20::make_script_engine();
    setup_val_class_comp_prop_test<true, true, false>(engine);
    check_val_class_comp_property(engine);
}

TEST(val_class_comp_prop, native_off_mp)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    using namespace test_bind;
    auto engine = asbind20::make_script_engine();
    setup_val_class_comp_prop_test<false, false, true>(engine);
    check_val_class_comp_property(engine);
}

TEST(val_class_comp_prop, generic_off_mp)
{
    using namespace test_bind;
    auto engine = asbind20::make_script_engine();
    setup_val_class_comp_prop_test<true, false, false>(engine);
    check_val_class_comp_property(engine);
}

TEST(val_class_comp_prop, native_mp_mp)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    using namespace test_bind;
    auto engine = asbind20::make_script_engine();
    setup_val_class_comp_prop_test<false, true, true>(engine);
    check_val_class_comp_property(engine);
}

TEST(val_class_comp_prop, generic_mp_mp)
{
    using namespace test_bind;
    auto engine = asbind20::make_script_engine();
    setup_val_class_comp_prop_test<true, true, true>(engine);
    check_val_class_comp_property(engine);
}
