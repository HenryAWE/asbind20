#include <gtest/gtest.h>
#include "shared.hpp"
#include <asbind20/asbind.hpp>
#include <asbind20/ext/helper.hpp>

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

    friend bool operator==(const my_value_class& lhs, const my_value_class& rhs)
    {
        return lhs.value == rhs.value;
    }

    friend std::strong_ordering operator<=>(const my_value_class& lhs, const my_value_class& rhs)
    {
        return lhs.value <=> rhs.value;
    }

    int get_val() const
    {
        return value;
    }

    void set_val(int new_val)
    {
        value = new_val;
    }

    my_value_class& operator++()
    {
        ++value;
        return *this;
    }

    my_value_class operator++(int)
    {
        my_value_class tmp(*this);
        ++*this;
        return tmp;
    }

    my_value_class& operator--()
    {
        --value;
        return *this;
    }

    my_value_class operator--(int)
    {
        my_value_class tmp(*this);
        --*this;
        return tmp;
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

void set_val_gen(asIScriptGeneric* gen)
{
    my_value_class* this_ = (my_value_class*)gen->GetObject();
    int new_val = asbind20::get_generic_arg<int>(gen, 0);

    this_->set_val(new_val);
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

using namespace asbind_test;

TEST_F(asbind_test_suite, value_class)
{
    asIScriptEngine* engine = get_engine();

    using test_bind::my_value_class;

    asbind20::value_class<my_value_class>(
        engine,
        "my_value_class",
        asOBJ_APP_CLASS_CDAK | asOBJ_APP_CLASS_ALLINTS | asOBJ_APP_CLASS_MORE_CONSTRUCTORS
    )
        .common_behaviours()
        .constructor<int>("void f(int val)")
        .opEquals()
        .opCmp()
        .opPreInc()
        .opPreDec()
        .opPostInc()
        .opPostDec()
        .method("void set_val(int)", &my_value_class::set_val)
        .method("void set_val2(int)", &test_bind::set_val_gen)
        .method("int get_val() const", &my_value_class::get_val)
        .method("void add(int val)", &test_bind::add_obj_last)
        .method("void mul(int val)", &test_bind::mul_obj_first)
        .method("void add2(int val)", &test_bind::add_obj_last_ref)
        .method("void mul2(int val)", &test_bind::mul_obj_first_ref)
        .property("int value", &my_value_class::value);

    asIScriptModule* m = engine->GetModule("test_value_class", asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "test_value_class.as",
        "int test_1()"
        "{"
        "my_value_class val;"
        "val.set_val(42);"
        "assert(val.value == 42);"
        "assert(val == my_value_class(42));"
        "return val.get_val();"
        "}"
        "int test_2()"
        "{"
        "my_value_class val;"
        "val.set_val2(182375);"
        "assert(val.value < 182376);"
        "assert(val < my_value_class(182376));"
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
        "my_value_class test_6()"
        "{"
        "my_value_class val(0);"
        "assert(++val == my_value_class(1));"
        "my_value_class tmp = val++;"
        "assert(tmp.value == 1);"
        "return val;"
        "}"
        "my_value_class test_7()"
        "{"
        "my_value_class val(2);"
        "assert(--val == my_value_class(1));"
        "print(to_string(val.value));"
        "my_value_class tmp = val--;"
        "assert(tmp.value == 1);"
        "return val;"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    asbind20::request_context ctx(engine);

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

    auto test_6 = asbind20::script_function<my_value_class()>(m->GetFunctionByName("test_6"));
    EXPECT_EQ(test_6(ctx).get_val(), 2);

    auto test_7 = asbind20::script_function<my_value_class()>(m->GetFunctionByName("test_7"));
    EXPECT_EQ(test_7(ctx).get_val(), 0);
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

TEST_F(asbind_test_suite, global)
{
    std::string val = "val";

    struct class_wrapper
    {
        int value = 0;

        void set_val(int val)
        {
            value = val;
        }
    };

    class_wrapper wrapper;

    asbind20::global(get_engine())
        .function(
            "int gen_int()",
            +[]() -> int
            { return 42; }
        )
        .function(
            "void set_val(int val)",
            &class_wrapper::set_val,
            wrapper
        )
        .property("string val", val);

    EXPECT_EQ(val, "val");
    asbind20::ext::exec(get_engine(), "val = \"new string\"");
    EXPECT_EQ(val, "new string");

    EXPECT_EQ(wrapper.value, 0);
    asbind20::ext::exec(get_engine(), "set_val(gen_int())");
    EXPECT_EQ(wrapper.value, 42);

    asIScriptContext* ctx = get_engine()->CreateContext();
    {
        asIScriptFunction* gen_int = get_engine()->GetGlobalFunctionByDecl("int gen_int()");
        auto result = asbind20::script_invoke<int>(ctx, gen_int);
        EXPECT_EQ(result.value(), 42);
    }
    ctx->Release();
}

TEST_F(asbind_test_suite, interface)
{
    asIScriptEngine* engine = get_engine();

    asbind20::interface(engine, "my_interface")
        .method("int get() const");

    asIScriptModule* m = engine->GetModule("test_interface", asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "test_interface.as",
        "class my_impl : my_interface"
        "{"
        "int get() const override { return 42; }"
        "};"
        "int test() { my_impl val; return val.get(); }"
    );
    ASSERT_GE(m->Build(), 0);

    asIScriptContext* ctx = engine->CreateContext();
    asIScriptFunction* func = m->GetFunctionByDecl("int test()");
    ASSERT_TRUE(func);

    auto result = asbind20::script_invoke<int>(ctx, func);
    ASSERT_TRUE(result_has_value(result));
    EXPECT_EQ(result.value(), 42);

    ctx->Release();

    m->Discard();
}

TEST_F(asbind_test_suite, funcdef_and_typedef)
{
    asIScriptEngine* engine = get_engine();

    asbind20::global(engine)
        .funcdef("bool callback(int, int)")
        .typedef_("float", "real32");

    asIScriptModule* m = engine->GetModule("test_def", asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "test_def.as",
        "bool pred(int a, int b) { return a < b; }"
        "void main() { callback@ cb = @pred; assert(cb(1, 2)); }"
        "real32 get_pi() { return 3.14f; }"
    );
    ASSERT_GE(m->Build(), 0);

    asIScriptContext* ctx = engine->CreateContext();
    {
        asIScriptFunction* func = m->GetFunctionByDecl("void main()");
        ASSERT_TRUE(func);

        auto result = asbind20::script_invoke<void>(ctx, func);
        ASSERT_TRUE(result_has_value(result));
    }

    {
        asIScriptFunction* func = m->GetFunctionByDecl("real32 get_pi()");
        ASSERT_TRUE(func);

        auto result = asbind20::script_invoke<float>(ctx, func);
        ASSERT_TRUE(result_has_value(result));
        EXPECT_FLOAT_EQ(result.value(), 3.14f);
    }
    ctx->Release();

    m->Discard();
}

namespace test_bind
{
int my_div(int a, int b)
{
    return a / b;
}
} // namespace test_bind

TEST_F(asbind_test_suite, generic)
{
    using namespace asbind20;

    asIScriptEngine* engine = get_engine();

    global(engine)
        .function("int my_div(int a, int b)", generic_wrapper<&test_bind::my_div, asCALL_CDECL>());

    asIScriptModule* m = engine->GetModule("test_generic", asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "test_def.as",
        "void main()"
        "{"
        "assert(my_div(6, 2) == 3);"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    {
        request_context ctx(engine);
        asIScriptFunction* func = m->GetFunctionByDecl("void main()");
        auto result = script_invoke<void>(ctx, func);
        EXPECT_TRUE(result_has_value(result));
    }

    m->Discard();
}

namespace test_bind
{
enum class my_enum : int
{
    A,
    B
};
} // namespace test_bind

TEST_F(asbind_test_suite, enum)
{
    asIScriptEngine* engine = get_engine();

    using test_bind::my_enum;

    asbind20::global(engine)
        .enum_type("my_enum")
        .enum_value("my_enum", my_enum::A, "A")
        .enum_value("my_enum", my_enum::B, "B");

    asIScriptModule* m = engine->GetModule("test_enum", asGM_ALWAYS_CREATE);

    m->AddScriptSection(
        "test_enum.as",
        "my_enum get_enum_val() { return my_enum::A; }"
        "bool check_enum_val(my_enum val) { return val == my_enum::B; }"
    );
    ASSERT_GE(m->Build(), 0);

    asIScriptContext* ctx = engine->CreateContext();
    {
        asIScriptFunction* func = m->GetFunctionByDecl("my_enum get_enum_val()");
        ASSERT_TRUE(func);

        auto result = asbind20::script_invoke<my_enum>(ctx, func);
        ASSERT_TRUE(result_has_value(result));
        EXPECT_EQ(result.value(), my_enum::A);
    }

    {
        asIScriptFunction* func = m->GetFunctionByDecl("bool check_enum_val(my_enum val)");
        ASSERT_TRUE(func);

        auto result = asbind20::script_invoke<bool>(ctx, func, my_enum::B);
        ASSERT_TRUE(result_has_value(result));
        EXPECT_TRUE(result.value());
    }
    ctx->Release();

    m->Discard();
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    std::cerr << "asGetLibraryVersion(): " << asGetLibraryVersion() << std::endl;
    std::cerr << "asGetLibraryOptions(): " << asGetLibraryOptions() << std::endl;
    std::cerr << "asbind20::library_version(): " << asbind20::library_version() << std::endl;
    std::cerr << "asbind20::library_options(): " << asbind20::library_options() << std::endl;
    return RUN_ALL_TESTS();
}
