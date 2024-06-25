#include <gtest/gtest.h>
#include "shared.hpp"
#include <asbind20/asbind.hpp>

namespace test_bind
{
class my_value_class
{
public:
    my_value_class() = default;
    my_value_class(const my_value_class&) = default;

    my_value_class(int val)
        : value(val) {}

    ~my_value_class() = default;

    my_value_class& operator=(const my_value_class&) = default;

    int get_val() const
    {
        return value;
    }

    void set_val(int new_val)
    {
        value = new_val;
    }

    int value = 0;
};

void add_obj_last(int val, my_value_class* this_)
{
    this_->value += val;
}

void mul_obj_first(my_value_class* this_, int val)
{
    this_->value *= val;
}

void add_obj_last_ref(int val, my_value_class& this_)
{
    this_.value += val;
}

void mul_obj_first_ref(my_value_class& this_, int val)
{
    this_.value *= val;
}

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
} // namespace test_bind

using asbind_test::asbind_test_suite;

TEST_F(asbind_test_suite, value_class)
{
    asIScriptEngine* engine = get_engine();

    using test_bind::my_value_class;

    asbind20::value_class<my_value_class>(engine, "my_value_class", asOBJ_APP_CLASS_CDAK)
        .common_behaviours()
        .constructor<int>("void f(int val)")
        .method("void set_val(int)", &my_value_class::set_val)
        .method("int get_val() const", &my_value_class::get_val)
        .method("void add(int val)", test_bind::add_obj_last)
        .method("void mul(int val)", test_bind::mul_obj_first)
        .method("void add2(int val)", test_bind::add_obj_last_ref)
        .method("void mul2(int val)", test_bind::mul_obj_first_ref)
        .property("int value", &my_value_class::value);

    asIScriptModule* m = engine->GetModule("test_value_class", asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "test_value_class.as",
        "int test_1()"
        "{"
        "my_value_class val;"
        "val.set_val(42);"
        "return val.get_val();"
        "}"
        "int test_2()"
        "{"
        "my_value_class val;"
        "val.set_val(182375);"
        "val.add(1);"
        "return val.get_val();"
        "}"
        "int test_3()"
        "{"
        "my_value_class val;"
        "val.set_val(2);"
        "val.mul(3);"
        "return val.get_val();"
        "}"
        "int test_4()"
        "{"
        "my_value_class val;"
        "val.set_val(2);"
        "val.add2(1);"
        "val.mul2(3);"
        "return val.get_val();"
        "}"
        "int test_5()"
        "{"
        "my_value_class val(4);"
        "val.value += 1;"
        "return val.value;"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    asIScriptContext* ctx = engine->CreateContext();
    {
        auto test_1 = asbind20::script_function<int()>(m->GetFunctionByName("test_1"));
        EXPECT_EQ(test_1(ctx), 42);

        auto test_2 = asbind20::script_function<int()>(m->GetFunctionByName("test_2"));
        EXPECT_EQ(test_2(ctx), 182376);

        auto test_3 = asbind20::script_function<int()>(m->GetFunctionByName("test_3"));
        EXPECT_EQ(test_3(ctx), 6);

        auto test_4 = asbind20::script_function<int()>(m->GetFunctionByName("test_4"));
        EXPECT_EQ(test_4(ctx), 9);

        auto test_5 = asbind20::script_function<int()>(m->GetFunctionByName("test_5"));
        EXPECT_EQ(test_5(ctx), 5);
    }
    ctx->Release();
}

TEST_F(asbind_test_suite, ref_class)
{
    asIScriptEngine* engine = get_engine();

    using test_bind::my_ref_class;

    asbind20::ref_class<my_ref_class>(engine, "my_ref_class")
        .default_factory()
        .factory("my_ref_class@ f(int)", &my_ref_class::create_by_val)
        .addref(&my_ref_class::addref)
        .release(&my_ref_class::release)
        .method("uint use_count() const", &my_ref_class::use_count)
        .method("int exchange_data(int new_data)", &test_bind::exchange_data)
        .property("int data", &my_ref_class::data);

    asIScriptModule* m = engine->GetModule("test_value_class", asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "test_ref_class.as",
        "int test_1()"
        "{"
        "my_ref_class val;"
        "return val.use_count();"
        "}"
        "int test_2()"
        "{"
        "my_ref_class val;"
        "my_ref_class@ val2 = val;"
        "return val.use_count();"
        "}"
        "int test_3()"
        "{"
        "my_ref_class val(2);"
        "int old = val.exchange_data(3);"
        "return old + val.data;"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    asIScriptContext* ctx = engine->CreateContext();
    {
        auto test_1 = asbind20::script_function<int()>(m->GetFunctionByName("test_1"));
        EXPECT_EQ(test_1(ctx), 1);

        auto test_2 = asbind20::script_function<int()>(m->GetFunctionByName("test_2"));
        EXPECT_EQ(test_2(ctx), 2);

        auto test_3 = asbind20::script_function<int()>(m->GetFunctionByName("test_3"));
        EXPECT_EQ(test_3(ctx), 5);
    }
    ctx->Release();
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    std::cerr << "asGetLibraryVersion(): " << asGetLibraryVersion() << std::endl;
    std::cerr << "asGetLibraryOptions(): " << asGetLibraryOptions() << std::endl;
    return RUN_ALL_TESTS();
}
