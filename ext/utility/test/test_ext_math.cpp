#include <cmath>
#include <gtest/gtest.h>
#include "asbind20/ext/math.hpp"
#include <shared_test_lib.hpp>
#include <numbers>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/stdstring.hpp>

using namespace asbind_test;

TEST(ext_math, close_to)
{
    using namespace asbind20;

    EXPECT_TRUE(ext::math_close_to<float>(
        std::numbers::pi_v<float>, 3.14f, 0.01f
    ));
}

TEST_F(asbind_test_suite, ext_math)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    run_file("script/test_math.as");
}

TEST_F(asbind_test_suite_generic, ext_math)
{
    run_file("script/test_math.as");
}

static void test_math_complex(asIScriptEngine* engine)
{
    asIScriptModule* m = engine->GetModule("math_complex", asGM_ALWAYS_CREATE);
    m->AddScriptSection(
        "math_complex.as",
        "void check1()\n"
        "{\n"
        "complex c;"
        "assert(c.real == 0);\n"
        "assert(c.imag == 0);\n"
        "}\n"
        "void check2()\n"
        "{\n"
        "complex c(1.0f);"
        "assert(close_to(c.real, 1.0f));\n"
        "assert(c.imag == 0);\n"
        "}\n"
        "void check3()\n"
        "{\n"
        "complex c = {1.0f, 2.0f};"
        "assert(close_to(c.real, 1.0f));\n"
        "assert(close_to(c.imag, 2.0f));\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    {
        asbind20::request_context ctx(engine);
        asIScriptFunction* f = m->GetFunctionByName("check1");
        ASSERT_TRUE(f);

        auto result = asbind20::script_invoke<void>(ctx, f);
        EXPECT_TRUE(result_has_value(result));
    }

    {
        asbind20::request_context ctx(engine);
        asIScriptFunction* f = m->GetFunctionByName("check2");
        ASSERT_TRUE(f);

        auto result = asbind20::script_invoke<void>(ctx, f);
        EXPECT_TRUE(result_has_value(result));
    }
}

TEST_F(asbind_test_suite, math_complex)
{
    if(asbind20::has_max_portability())
        GTEST_SKIP() << "AS_MAX_PORTABILITY";

    asIScriptEngine* engine = get_engine();
    asbind20::ext::register_math_complex(engine);

    test_math_complex(engine);
}

TEST_F(asbind_test_suite_generic, math_complex)
{
    asIScriptEngine* engine = get_engine();
    asbind20::ext::register_math_complex(engine, true);

    test_math_complex(engine);
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
