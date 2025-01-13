#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>

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

    asUINT use_count() const
    {
        return m_use_count;
    }

    int data = 0;

private:
    asUINT m_use_count = 1;
};

int exchange_data(my_ref_class& this_, int new_data)
{
    return std::exchange(this_.data, new_data);
}

void get_ref_class_data(asIScriptGeneric* gen)
{
    using namespace asbind20;

    const my_ref_class* this_ = get_generic_object<const my_ref_class*>(gen);
    set_generic_return<int>(gen, (int)this_->data);
}

void register_ref_class(asIScriptEngine* engine)
{
    using namespace asbind20;

    ref_class<my_ref_class> c(engine, "my_ref_class");
    c
        .default_factory()
        .factory_function<&my_ref_class::create_by_val>("int", use_explicit)
        .list_factory<int>("int,int")
        .addref(&my_ref_class::addref)
        .release(&my_ref_class::release)
        .opAddAssign()
        .method("uint use_count() const", &my_ref_class::use_count)
        .method("int exchange_data(int new_data)", &test_bind::exchange_data)
        .method("int get_data() const", &get_ref_class_data)
        .property("int data", &my_ref_class::data);
}

void register_ref_class(asbind20::use_generic_t, asIScriptEngine* engine)
{
    using namespace asbind20;

    ref_class<my_ref_class, true> c(engine, "my_ref_class");
    c
        .default_factory()
        .factory_function<&my_ref_class::create_by_val>("int", use_explicit)
        .list_factory<int>("int,int")
        .addref<&my_ref_class::addref>()
        .release<&my_ref_class::release>()
        .opAddAssign()
        .method<&my_ref_class::use_count>("uint use_count() const")
        .method<&test_bind::exchange_data>("int exchange_data(int new_data)")
        .method("int get_data() const", &get_ref_class_data)
        .property("int data", &my_ref_class::data);
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
)";

static void check_ref_class(asIScriptEngine* engine)
{
    using test_bind::my_ref_class;

    asIScriptModule* m = engine->GetModule("test_value_class", asGM_ALWAYS_CREATE);

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
}

TEST_F(asbind_test_suite, ref_class)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    asIScriptEngine* engine = get_engine();
    register_ref_class(engine);

    check_ref_class(engine);
}

TEST_F(asbind_test_suite_generic, ref_class)
{
    asIScriptEngine* engine = get_engine();
    register_ref_class(asbind20::use_generic, engine);

    check_ref_class(engine);
}
