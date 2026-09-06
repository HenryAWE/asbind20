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
} // namespace

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

void check_reflected_global(asbind20::engine_pointer engine)
{
    // Reset global value
    prop = 0;

    auto m = asbind20::create_module(engine, "refl_global");
    m->AddScriptSection(
        "refl_global",
        "int run0() { return global_fn(13); }\n"
        "int run1() { prop = 13; return prop + c_prop; }"
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
}
} // namespace

TEST(Reflection, GlobalNative)
{
    ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

    using namespace asbind20;
    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);

    global<false>(engine)
        .function(reflect<^^global_fn>())
        .property(reflect<^^c_prop>())
        .property(reflect<^^prop>());

    check_reflected_global(engine);
}

TEST(Reflection, GlobalGeneric)
{
    using namespace asbind20;
    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);

    global<true>(engine)
        .function(reflect<^^global_fn>())
        .property(reflect<^^c_prop>())
        .property(reflect<^^prop>());

    check_reflected_global(engine);
}

#endif
