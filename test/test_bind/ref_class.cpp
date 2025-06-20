#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/assert.hpp>

namespace test_bind
{
class my_ref_class
{
public:
    static my_ref_class* create_by_val(int val)
    {
        return new my_ref_class(val);
    }

    my_ref_class() = default;

    my_ref_class(int val)
        : data(val) {}

    // Expects the size of `arr` is 2
    my_ref_class(int* arr)
        : data(arr[0] + arr[1]) {}

    my_ref_class& operator+=(const my_ref_class& rhs)
    {
        data += rhs.data;
        return *this;
    }

    my_ref_class& operator%=(const my_ref_class& rhs)
    {
        data %= rhs.data;
        return *this;
    }

    operator int() const
    {
        return data;
    }

    explicit operator bool() const
    {
        return data != 0;
    }

    void addref()
    {
        m_use_count += 1;
    }

    void release()
    {
        assert(m_use_count != 0);

        m_use_count -= 1;
        if(m_use_count == 0)
            delete this;
    }

    AS_NAMESPACE_QUALIFIER asUINT use_count() const
    {
        return m_use_count;
    }

    void from_var_type(void* ref, int type_id)
    {
        if(asbind20::is_void_type(type_id) || !asbind20::is_primitive_type(type_id))
            return;

        asbind20::visit_primitive_type(
            [this](auto* ref)
            {
                data = static_cast<int>(*ref);
            },
            type_id,
            ref
        );
    }

    int data = 0;

private:
    AS_NAMESPACE_QUALIFIER asUINT m_use_count = 1;
};

static int exchange_data(my_ref_class& this_, int new_data)
{
    return std::exchange(this_.data, new_data);
}

static void get_ref_class_data(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
{
    using namespace asbind20;

    const my_ref_class* this_ = get_generic_object<const my_ref_class*>(gen);
    set_generic_return<int>(gen, (int)this_->data);
}

void register_ref_class(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    ref_class<my_ref_class> c(engine, "my_ref_class");
    c
        .default_factory()
        .factory_function("int", use_explicit, &my_ref_class::create_by_val)
        .list_factory<int>("int,int")
        .addref(&my_ref_class::addref)
        .release(&my_ref_class::release)
        .opAddAssign()
        .opModAssign()
        .opConv<bool>()
        .opImplConv<int>()
        .method("uint use_count() const", &my_ref_class::use_count)
        .method("int exchange_data(int new_data)", &test_bind::exchange_data)
        .method("int get_data() const", &get_ref_class_data)
        .method("void from_var_type(const ?&in)", &my_ref_class::from_var_type)
        .method(
            "void set_by_lambda()",
            [](my_ref_class& c)
            { c.data = 1013; }
        )
        .property("int data", &my_ref_class::data);

    EXPECT_EQ(c.get_engine(), engine);
    EXPECT_FALSE(c.force_generic());
}

void register_ref_class(asbind20::use_generic_t, AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    ref_class<my_ref_class, true> c(engine, "my_ref_class");
    c
        .default_factory()
        .factory_function("int", use_explicit, fp<&my_ref_class::create_by_val>)
        .list_factory<int>("int,int")
        .addref(fp<&my_ref_class::addref>)
        .release(fp<&my_ref_class::release>)
        .opAddAssign()
        .opModAssign()
        .opConv<bool>()
        .opImplConv<int>()
        .method("uint use_count() const", fp<&my_ref_class::use_count>)
        .method("int exchange_data(int new_data)", fp<&test_bind::exchange_data>)
        .method("int get_data() const", &get_ref_class_data)
        .method("void from_var_type(const ?&in)", fp<&my_ref_class::from_var_type>, var_type<0>)
        .method(
            "void set_by_lambda()",
            [](my_ref_class& c)
            { c.data = 1013; }
        )
        .property("int data", &my_ref_class::data);

    EXPECT_EQ(c.get_engine(), engine);
    EXPECT_TRUE(c.force_generic());
}
} // namespace test_bind

using namespace test_bind;
using namespace asbind_test;

const char* ref_value_class_test_script = R"(
int test_0()
{
    my_ref_class val;
    return val.get_data();
}
int test_1()
{
    my_ref_class val;
    return val.use_count();
}
int test_2()
{
    my_ref_class val;
    my_ref_class@ val2 = val;
    return val.use_count();
}
int test_3()
{
    my_ref_class val(2);
    int old = val.exchange_data(3);
    return old + val.data;
}
int test_4()
{
    my_ref_class val = {3, 4};
    int internal = val;
    assert(internal == 7);
    assert(bool(val));
    return val.data;
}
int test_5()
{
    my_ref_class val1(1);
    my_ref_class val2(2);
    my_ref_class@ ref = val2 += val1;
    assert(ref is @val2);
    return val2.data;
}
int test_6()
{
    my_ref_class val(0);
    val.set_by_lambda();
    return val.data;
}
int test_7()
{
    my_ref_class val(0);
    val.from_var_type(3.14);
    assert(val.data == 3);
    val.from_var_type(true);
    return val.data;
}
int test_8()
{
    my_ref_class val1(3);
    my_ref_class val2(4);
    my_ref_class@ ref = val2 %= val1;
    assert(ref is @val2);
    return val2.data;
}
)";

static void check_ref_class(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using test_bind::my_ref_class;

    auto* m = engine->GetModule("test_value_class", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "test_ref_class.as",
        ref_value_class_test_script
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
    check_int_result(1, 1);
    check_int_result(2, 2);
    check_int_result(3, 5);
    check_int_result(4, 7);
    check_int_result(5, 3);
    check_int_result(6, 1013);
    check_int_result(7, 1);
    check_int_result(8, 1);
}

TEST_F(asbind_test_suite, ref_class)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    auto* engine = get_engine();
    register_ref_class(engine);

    check_ref_class(engine);
}

TEST_F(asbind_test_suite_generic, ref_class)
{
    auto* engine = get_engine();
    register_ref_class(asbind20::use_generic, engine);

    check_ref_class(engine);
}

namespace test_bind
{
class ref_class_for_helper
{
public:
    void addref()
    {
        ++m_counter;
    }

    void release()
    {
        assert(m_counter != 0);
        if(--m_counter == 0)
            delete this;
    }

    ref_class_for_helper(int val)
        : value(val) {}

    int value;

private:
    ~ref_class_for_helper() = default;

    int m_counter = 1;
};

struct ref_helper
{
    int predefined_value = 0;

    int by_functor_objfirst(ref_class_for_helper& this_, int additional)
    {
        this_.value += additional;
        return std::exchange(predefined_value, 0);
    }

    int by_functor_objlast(int additional, ref_class_for_helper& this_)
    {
        this_.value += additional;
        return std::exchange(predefined_value, 0);
    }

    int by_functor_objfirst_var(ref_class_for_helper& this_, int additional, void* ref, int type_id)
    {
        this_.value += additional;
        if(type_id == AS_NAMESPACE_QUALIFIER asTYPEID_INT32)
            this_.value += *(int*)ref;
        return std::exchange(predefined_value, 0);
    }

    int by_functor_objlast_var(int additional, void* ref, int type_id, ref_class_for_helper& this_)
    {
        this_.value += additional;
        if(type_id == AS_NAMESPACE_QUALIFIER asTYPEID_INT32)
            this_.value += *(int*)ref;
        return std::exchange(predefined_value, 0);
    }
};

template <bool UseGeneric>
static void register_ref_class_for_helper(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, ref_helper& helper)
{
    using namespace asbind20;

    ref_class<ref_class_for_helper, UseGeneric> c(
        engine,
        "ref_class_for_helper"
    );

    c
        .template factory<int>("int")
        .addref(fp<&ref_class_for_helper::addref>)
        .release(fp<&ref_class_for_helper::release>)
        .method("int by_functor_objfirst(int)", fp<&ref_helper::by_functor_objfirst>, auxiliary(helper))
        .method("int by_functor_objlast(int)", fp<&ref_helper::by_functor_objlast>, auxiliary(helper))
        .method("int by_functor_objfirst_var(int, const ?&in)", fp<&ref_helper::by_functor_objfirst_var>, var_type<1>, auxiliary(helper))
        .method("int by_functor_objlast_var(int, const ?&in)", fp<&ref_helper::by_functor_objlast_var>, var_type<1>, auxiliary(helper))
        .property("int value", &ref_class_for_helper::value);
}

static void check_ref_class_for_helper(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, ref_helper& helper)
{
    using test_bind::ref_class_for_helper;

    auto* m = engine->GetModule("test_value_class", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "test_ref_class.as",
        "int test0() { ref_class_for_helper val(1000); assert(val.by_functor_objfirst(13) == 182375); return val.value; }"
        "int test1() { ref_class_for_helper val(1000); assert(val.by_functor_objlast(13) == 182376); return val.value; }"
        "int test2() { ref_class_for_helper val(1000); assert(val.by_functor_objfirst_var(10, 3) == 182375); return val.value; }"
        "int test3() { ref_class_for_helper val(1000); assert(val.by_functor_objfirst_var(10, 3) == 182376); return val.value; }"
    );
    ASSERT_GE(m->Build(), 0);

    auto check_int_result = [&](int idx, int expected_val) -> void
    {
        std::string test_name = asbind20::string_concat("test", std::to_string(idx));
        auto test_case = asbind20::script_function<int()>(m->GetFunctionByName(test_name.c_str()));

        asbind20::request_context ctx(engine);
        auto result = test_case(ctx);
        ASSERT_TRUE(result_has_value(result));
        EXPECT_EQ(*result, expected_val)
            << test_name;
    };

    helper.predefined_value = 182375;
    check_int_result(0, 1013);
    EXPECT_EQ(helper.predefined_value, 0);

    helper.predefined_value = 182376;
    check_int_result(1, 1013);
    EXPECT_EQ(helper.predefined_value, 0);

    helper.predefined_value = 182375;
    check_int_result(2, 1013);
    EXPECT_EQ(helper.predefined_value, 0);

    helper.predefined_value = 182376;
    check_int_result(3, 1013);
    EXPECT_EQ(helper.predefined_value, 0);
}
} // namespace test_bind

TEST_F(asbind_test_suite, ref_class_for_helper)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    auto* engine = get_engine();
    test_bind::ref_helper helper;
    test_bind::register_ref_class_for_helper<false>(engine, helper);

    test_bind::check_ref_class_for_helper(engine, helper);
}

TEST_F(asbind_test_suite_generic, ref_class_for_helper)
{
    auto* engine = get_engine();
    test_bind::ref_helper helper;
    test_bind::register_ref_class_for_helper<true>(engine, helper);

    test_bind::check_ref_class_for_helper(engine, helper);
}

namespace test_bind
{
struct ref_class_comp_data
{
    int comp_a;
    int comp_b;
};

class base_ref_class
{
public:
    base_ref_class()
        : a(0), indirect(new ref_class_comp_data{1, 2}), b(3) {}

    ~base_ref_class()
    {
        delete indirect;
    }

    void addref()
    {
        ++counter;
    }

    void release()
    {
        --counter;
        if(counter == 0)
            delete this;
    }

    int a;
    ref_class_comp_data* const indirect;
    int b;

    int counter = 1;
};

template <bool UseGeneric, bool UseMP, bool CompUseMP>
static void register_base_ref_class(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using asbind20::composite;
    using asbind20::fp;

    asbind20::ref_class<base_ref_class, UseGeneric> c(engine, "base_ref_class");
    c
        .default_factory()
        .addref(fp<&base_ref_class::addref>)
        .release(fp<&base_ref_class::release>)
        .property("int a", &base_ref_class::a)
        .property("int b", &base_ref_class::b);

    if constexpr(UseMP && CompUseMP)
    {
        c
            .property("int comp_a", &ref_class_comp_data::comp_a, composite(&base_ref_class::indirect))
            .property("int comp_b", &ref_class_comp_data::comp_b, composite(&base_ref_class::indirect));
    }
    else if constexpr(UseMP)
    {
        c
            .property("int comp_a", &ref_class_comp_data::comp_a, composite(offsetof(base_ref_class, indirect)))
            .property("int comp_b", &ref_class_comp_data::comp_b, composite(offsetof(base_ref_class, indirect)));
    }
    else if constexpr(CompUseMP)
    {
        c
            .property("int comp_a", offsetof(ref_class_comp_data, comp_a), composite(&base_ref_class::indirect))
            .property("int comp_b", offsetof(ref_class_comp_data, comp_b), composite(&base_ref_class::indirect));
    }
    else // Both are represented by offset
    {
        c
            .property("int comp_a", offsetof(ref_class_comp_data, comp_a), composite(offsetof(base_ref_class, indirect)))
            .property("int comp_b", offsetof(ref_class_comp_data, comp_b), composite(offsetof(base_ref_class, indirect)));
    }
}

static void check_ref_class_comp_property(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = engine->GetModule(
        "ref_class_comp_prop", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );
    ASSERT_TRUE(m);

    m->AddScriptSection(
        "test_comp_prop",
        "base_ref_class@ create_val() { return base_ref_class(); }\n"
        "void test()\n"
        "{\n"
        "    base_ref_class c;\n"
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
        auto result = asbind20::script_invoke<base_ref_class*>(ctx, f);
        ASSERT_TRUE(asbind_test::result_has_value(result));
        ASSERT_NE(*result, nullptr);
        EXPECT_EQ((*result)->a, 0);
        EXPECT_EQ((*result)->indirect->comp_a, 1);
        EXPECT_EQ((*result)->indirect->comp_b, 2);
        EXPECT_EQ((*result)->b, 3);
    }

    {
        auto* f = m->GetFunctionByName("test");

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<void>(ctx, f);
        EXPECT_TRUE(asbind_test::result_has_value(result));
    }
}

template <bool UseGeneric, bool UseMP, bool CompUseMP>
static void setup_ref_class_comp_prop_test(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    ext::register_script_assert(
        engine,
        [](std::string_view msg)
        {
            FAIL() << "ref_class_comp_prop failed: " << msg;
        }
    );

    register_base_ref_class<UseGeneric, UseMP, CompUseMP>(engine);
}
} // namespace test_bind

TEST(ref_class_comp_prop, native_off_off)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    using namespace test_bind;
    auto engine = asbind20::make_script_engine();
    setup_ref_class_comp_prop_test<false, false, false>(engine);
    check_ref_class_comp_property(engine);
}

TEST(ref_class_comp_prop, generic_off_off)
{
    using namespace test_bind;
    auto engine = asbind20::make_script_engine();
    setup_ref_class_comp_prop_test<true, false, false>(engine);
    check_ref_class_comp_property(engine);
}

TEST(ref_class_comp_prop, native_mp_off)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    using namespace test_bind;
    auto engine = asbind20::make_script_engine();
    setup_ref_class_comp_prop_test<false, true, false>(engine);
    check_ref_class_comp_property(engine);
}

TEST(ref_class_comp_prop, generic_mp_off)
{
    using namespace test_bind;
    auto engine = asbind20::make_script_engine();
    setup_ref_class_comp_prop_test<true, true, false>(engine);
    check_ref_class_comp_property(engine);
}

TEST(ref_class_comp_prop, native_off_mp)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    using namespace test_bind;
    auto engine = asbind20::make_script_engine();
    setup_ref_class_comp_prop_test<false, false, true>(engine);
    check_ref_class_comp_property(engine);
}

TEST(ref_class_comp_prop, generic_off_mp)
{
    using namespace test_bind;
    auto engine = asbind20::make_script_engine();
    setup_ref_class_comp_prop_test<true, false, false>(engine);
    check_ref_class_comp_property(engine);
}

TEST(ref_class_comp_prop, native_mp_mp)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    using namespace test_bind;
    auto engine = asbind20::make_script_engine();
    setup_ref_class_comp_prop_test<false, true, true>(engine);
    check_ref_class_comp_property(engine);
}

TEST(ref_class_comp_prop, generic_mp_mp)
{
    using namespace test_bind;
    auto engine = asbind20::make_script_engine();
    setup_ref_class_comp_prop_test<true, true, true>(engine);
    check_ref_class_comp_property(engine);
}
