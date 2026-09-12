#include <asbind_test/framework.hpp>
#include <asbind20/meta/reflection.hpp>

#ifdef ASBIND20_HAS_LIB_REFLECTION

#    if defined(__GNUC__) && !defined(__clang__)
// False positive if functions are only used in reflection
#        pragma GCC diagnostic ignored "-Wunused-function"
#    endif

namespace
{
int func0()
{
    return 0;
}

int func1(std::int8_t arg0, float arg1)
{
    (void)arg0;
    (void)arg1;
    return 0;
}

unsigned int& func2(const std::int8_t& arg0)
{
    (void)arg0;
    std::terminate();
}

struct my_struct
{
    int mem_func(float f_arg)
    {
        (void)f_arg;
        std::terminate();
    }

    int c_mem_func(float f_arg) const
    {
        (void)f_arg;
        std::terminate();
    }
};

namespace prefix
{
    struct my_type
    {};
} // namespace prefix
} // namespace

TEST(Reflection, TypeName)
{
    using namespace asbind20;

    EXPECT_EQ(
        meta::detail::calc_type_name<^^prefix::my_type>(),
        "my_type"
    );
}

TEST(Reflection, FuncSig)
{
    EXPECT_EQ(
        asbind20::meta::refl_function_sig<^^func0>(),
        "int func0()"
    );
    EXPECT_EQ(
        asbind20::meta::refl_function_sig<^^func1>(),
        "int func1(int8 arg0,float arg1)"
    );
    EXPECT_EQ(
        asbind20::meta::refl_function_sig<^^func2>(),
        "uint& func2(const int8&in arg0)"
    );
    EXPECT_EQ(
        asbind20::meta::refl_function_sig<^^my_struct::mem_func>(),
        "int mem_func(float f_arg)"
    );
    EXPECT_EQ(
        asbind20::meta::refl_function_sig<^^my_struct::c_mem_func>(),
        "int c_mem_func(float f_arg)const"
    );
    EXPECT_EQ(
        asbind20::meta::refl_function_sig<^^my_struct::c_mem_func>(true),
        "int c_mem_func(float f_arg)"
    );
}

namespace
{
int helper(int)
{
    return 1013;
}

[[maybe_unused]]
int global_prop = 0;

[[maybe_unused]]
const int c_global_prop = 0;
} // namespace

TEST(Reflection, Proxy)
{
    using asbind20::reflect;

    {
        auto proxy = reflect<^^helper>();
        EXPECT_EQ(
            proxy.get_decl(),
            "int helper(int)"
        );
        EXPECT_EQ(
            proxy.get_func(),
            &helper
        );
        EXPECT_EQ(
            std::invoke(proxy.get_func(), 0),
            1013
        );

        EXPECT_EQ(
            std::invoke(asbind20::fp<proxy.get_func()>.get(), 0),
            1013
        );
    }

    {
        auto proxy = reflect<^^global_prop>();
        EXPECT_EQ(proxy.get_decl(), "int global_prop");
        EXPECT_EQ(proxy.get_addr(), &global_prop);
    }

    {
        auto proxy = reflect<^^c_global_prop>();
        EXPECT_EQ(proxy.get_decl(), "const int c_global_prop");
        EXPECT_EQ(proxy.get_addr(), &c_global_prop);
    }
}

namespace
{
int global_fn(int arg)
{
    return 1000 + arg;
}

int prop = 0;
const int c_prop = 1000;

struct wrapper
{
    int val;

    int get() const
    {
        return val;
    }

    void set(int v)
    {
        val = v;
    }
};

void check_reflected_global(asbind20::engine_pointer engine, wrapper& w)
{
    // Reset global & wrapper value
    prop = 0;
    w.val = 3;

    auto m = asbind20::create_module(engine, "refl_global");
    m->AddScriptSection(
        "refl_global",
        "int run0() { return global_fn(13); }\n"
        "int run1() { prop = 13; return prop + c_prop; }\n"
        "int run2() { int old = get(); set(4); return old + 4; }"
    );
    ASSERT_GE(m->Build(), 0);

    {
        auto* run0 = m->GetFunctionByName("run0");
        ASSERT_THAT(run0, ::testing::NotNull());
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, run0);
        ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
        EXPECT_EQ(result.value(), 1013);
    }

    {
        EXPECT_EQ(prop, 0)
            << "global property is set";

        auto* run1 = m->GetFunctionByName("run1");
        ASSERT_THAT(run1, ::testing::NotNull());
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, run1);
        ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
        EXPECT_EQ(prop, 13)
            << "global property is not set";
        EXPECT_EQ(result.value(), 1013);
    }

    {
        EXPECT_EQ(w.val, 3)
            << "wrapper property is set";

        auto* run2 = m->GetFunctionByName("run2");
        ASSERT_THAT(run2, ::testing::NotNull());
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, run2);
        ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
        EXPECT_EQ(w.val, 4)
            << "wrapper property is not set";
        EXPECT_EQ(result.value(), 7);
    }
}
} // namespace

TEST(Reflection, GlobalNative)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    using namespace asbind20;
    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);

    wrapper w{};
    global<false>(engine)
        .function(reflect<^^global_fn>())
        .property(reflect<^^c_prop>())
        .property(reflect<^^prop>())
        .function(reflect<^^wrapper::get>(), auxiliary(w))
        .function(reflect<^^wrapper::set>(), auxiliary(w));

    check_reflected_global(engine, w);
}

TEST(Reflection, GlobalGeneric)
{
    using namespace asbind20;

    wrapper w{};
    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);

    global<true>(engine)
        .function(reflect<^^global_fn>())
        .property(reflect<^^c_prop>())
        .property(reflect<^^prop>())
        .function(reflect<^^wrapper::get>(), auxiliary(w))
        .function(reflect<^^wrapper::set>(), auxiliary(w));

    check_reflected_global(engine, w);
}

namespace
{
class my_class
{
public:
    my_class()
        : val0(4), val1(3.14f) {}

    int val0;
    float val1;

    int mem_f() const
    {
        return 1013;
    }
};

void helper_setter(my_class& c)
{
    c.val0 = 42;
}

int helper_getter(const my_class* c, int val)
{
    EXPECT_EQ(val, 7);
    return c->val0 + static_cast<int>(c->val1);
}

void check_registered_my_class_interface(asbind20::typeinfo_pointer ti)
{
    ASSERT_THAT(ti, ::testing::NotNull());
    EXPECT_STREQ(ti->GetName(), "my_class");

    {
        auto* helper_getter_fp = ti->GetMethodByName("helper_getter");
        ASSERT_THAT(helper_getter_fp, ::testing::NotNull());

        EXPECT_THAT(
            helper_getter_fp->GetDeclaration(),
            ::testing::HasSubstr("const")
        );
        EXPECT_TRUE(helper_getter_fp->IsReadOnly());
    }
}

void check_reflected_val_class(asbind20::engine_pointer engine)
{
    using namespace asbind20;

    auto* m = create_module(engine, "my_class_test");
    ASSERT_THAT(m, ::testing::NotNull());
    m->AddScriptSection(
        "my_class_test",
        "int run0() { my_class c; return c.val0 + int(c.val1); }\n"
        "int run1() { my_class c; return c.mem_f(); }\n"
        "int run2() { my_class c; c.helper_setter(); return c.val0; }\n"
        "int run3() { my_class c; return c.helper_getter(7); }"
    );
    ASSERT_GE(m->Build(), 0);

    {
        auto* run0 = m->GetFunctionByName("run0");
        ASSERT_THAT(run0, ::testing::NotNull());
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, run0);
        ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
        EXPECT_EQ(result.value(), 7);
    }

    {
        auto* run1 = m->GetFunctionByName("run1");
        ASSERT_THAT(run1, ::testing::NotNull());
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, run1);
        ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
        EXPECT_EQ(result.value(), 1013);
    }

    {
        auto* run2 = m->GetFunctionByName("run2");
        ASSERT_THAT(run2, ::testing::NotNull());
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, run2);
        ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
        EXPECT_EQ(result.value(), 42);
    }

    {
        auto* run3 = m->GetFunctionByName("run3");
        ASSERT_THAT(run3, ::testing::NotNull());
        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<int>(ctx, run3);
        ASBIND_TEST_EXPECT_INVOKE_RESULT(result);
        EXPECT_EQ(result.value(), 7);
    }
}
} // namespace

TEST(Reflection, ClassNative)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    using namespace asbind20;
    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);

    value_class<my_class, false>(
        engine, "my_class", AS_NAMESPACE_QUALIFIER asOBJ_POD
    )
        .behaviours_by_traits()
        .property(reflect<^^my_class::val0>())
        .property(reflect<^^my_class::val1>())
        .method(reflect<^^my_class::mem_f>())
        .method(reflect<^^helper_setter>())
        .method(reflect<^^helper_getter>());

    check_registered_my_class_interface(
        engine->GetTypeInfoByName("my_class")
    );
    check_reflected_val_class(engine);
}

TEST(Reflection, ClassGeneric)
{
    using namespace asbind20;
    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);

    value_class<my_class, true>(
        engine, "my_class", AS_NAMESPACE_QUALIFIER asOBJ_POD
    )
        .behaviours_by_traits()
        .property(reflect<^^my_class::val0>())
        .property(reflect<^^my_class::val1>())
        .method(reflect<^^my_class::mem_f>())
        .method(reflect<^^helper_setter>())
        .method(reflect<^^helper_getter>());

    check_registered_my_class_interface(
        engine->GetTypeInfoByName("my_class")
    );
    check_reflected_val_class(engine);
}

namespace
{
enum my_enum0 : int
{
    zero = 0,
    one = 1
};

enum my_scoped_enum0 : int
{
    scoped_zero = 0,
    scoped_one = 1
};
} // namespace

TEST(Reflection, Enum)
{
    using namespace asbind20;
    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);

    enum_<my_enum0>(engine, "my_enum0")
        .value(reflect<^^my_enum0::zero>())
        .value(reflect<^^my_enum0::one>());

    enum_<my_scoped_enum0>(engine, "my_scoped_enum0")
        .value(reflect<^^my_scoped_enum0::scoped_zero>())
        .value(reflect<^^my_scoped_enum0::scoped_one>());

    {
        auto ti = engine->GetTypeInfoByName("my_enum0");
        ASSERT_THAT(ti, ::testing::NotNull());
        EXPECT_EQ(ti->GetEnumValueCount(), 2);

        compat::script_enum_value_type val;
        cstring_ref str = ti->GetEnumValueByIndex(0, &val);
        EXPECT_EQ(str, "zero");
        EXPECT_EQ(static_cast<my_enum0>(val), my_enum0::zero);
    }

    {
        auto ti = engine->GetTypeInfoByName("my_scoped_enum0");
        ASSERT_THAT(ti, ::testing::NotNull());
        EXPECT_EQ(ti->GetEnumValueCount(), 2);

        compat::script_enum_value_type val;
        cstring_ref str = ti->GetEnumValueByIndex(0, &val);
        EXPECT_EQ(str, "scoped_zero");
        EXPECT_EQ(static_cast<my_enum0>(val), my_scoped_enum0::scoped_zero);
    }
}

#endif
