#include <cmath>
#include <numbers>
#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/stdstring.hpp>
#include <asbind20/ext/assert.hpp>
#include <asbind20/ext/math.hpp>

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

namespace test_ext_math
{
static void check_math_complex(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    auto* m = engine->GetModule(
        "math_complex", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
    );

    m->AddScriptSection(
        "math_complex.as",
        // Default initialization
        "void check1()\n"
        "{\n"
        "    complex<float> cf;\n"
        "    assert(cf.real == 0);\n"
        "    assert(cf.imag == 0);\n"
        "    complex<double> cd;\n"
        "    assert(cd.real == 0);\n"
        "    assert(cd.imag == 0);\n"
        "}\n"
        // Real part only
        "void check2()\n"
        "{\n"
        "    complex<float> cf(1.0f);\n"
        "    assert(close_to(cf.real, 1.0f));\n"
        "    assert(cf.imag == 0);\n"
        "    complex<double> cd(1.0);\n"
        "    assert(close_to(cd.real, 1.0));\n"
        "    assert(cd.imag == 0);\n"
        "}\n"
        // Both parts
        "void check3()\n"
        "{\n"
        "    complex<float> cf(1.0f, 2.0f);\n"
        "    assert(close_to(cf.real, 1.0f));\n"
        "    assert(close_to(cf.imag, 2.0f));\n"
        "    complex<double> cd(1.0, 2.0);\n"
        "    assert(close_to(cd.real, 1.0));\n"
        "    assert(close_to(cd.imag, 2.0));\n"
        "}\n"
        // List initialization
        "void check4()\n"
        "{\n"
        "    complex<float> cf = {1.0f, 2.0f};\n"
        "    assert(close_to(cf.real, 1.0f));\n"
        "    assert(close_to(cf.imag, 2.0f));\n"
        "    complex<double> cd = {1.0, 2.0};\n"
        "    assert(close_to(cd.real, 1.0));\n"
        "    assert(close_to(cd.imag, 2.0));\n"
        "}\n"
        // Copying
        "void check5()\n"
        "{\n"
        "    complex<float> cf_src = {1.0f, 2.0f};\n"
        "    complex<float> cf = cf_src;\n"
        "    assert(close_to(cf.real, 1.0f));\n"
        "    assert(close_to(cf.imag, 2.0f));\n"
        "    complex<double> cd_src = {1.0, 2.0};\n"
        "    complex<double> cd = cd_src;\n"
        "    assert(close_to(cd.real, 1.0));\n"
        "    assert(close_to(cd.imag, 2.0));\n"
        "}\n"
        // Interchanging data between different element types
        "void check6()\n"
        "{\n"
        "    complex<float> cf_src = {1.0f, 2.0f};\n"
        "    complex<double> cd_src = {1.0, 2.0};\n"
        "    complex<float> cf = cd_src;\n"
        "    complex<double> cd = cf_src;\n"
        "    assert(close_to(cf.real, 1.0f));\n"
        "    assert(close_to(cf.imag, 2.0f));\n"
        "    assert(close_to(cd.real, 1.0));\n"
        "    assert(close_to(cd.imag, 2.0));\n"
        "}\n"
        // Members
        "void check7()\n"
        "{\n"
        "    complex<float> cf = {3.0f, 4.0f};\n"
        "    assert(close_to(cf.squared_length, 25.0f));\n"
        "    assert(close_to(cf.length, 5.0f, 0.000001f));\n"
        "    assert(close_to(abs(cf), 5.0f, 0.000001f));\n"
        "    complex<float> cd = {3.0, 4.0};\n"
        "    assert(close_to(cd.squared_length, 25.0));\n"
        "    assert(close_to(cd.length, 5.0, 0.000001));\n"
        "    assert(close_to(abs(cd), 5.0, 0.000001));\n"
        "}"
    );
    ASSERT_GE(m->Build(), 0);

    auto check = [&](int idx)
    {
        std::string func_name = asbind20::string_concat(
            "check", std::to_string(idx)
        );
        SCOPED_TRACE("func_name: " + func_name);

        asbind20::request_context ctx(engine);
        auto* f = m->GetFunctionByName(func_name.c_str());
        ASSERT_TRUE(f);

        auto result = asbind20::script_invoke<void>(ctx, f);
        EXPECT_TRUE(result_has_value(result));
    };

    for(int i = 1; i <= 7; ++i)
        check(i);
}

void check_complex_template(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    AS_NAMESPACE_QUALIFIER asITypeInfo* ti = nullptr;

    ti = engine->GetTypeInfoByDecl("complex<int>");
    EXPECT_EQ(ti, nullptr);
    ti = engine->GetTypeInfoByDecl("complex<bool>");
    EXPECT_EQ(ti, nullptr);
    ti = engine->GetTypeInfoByDecl("complex<void>");
    EXPECT_EQ(ti, nullptr);

    ti = engine->GetTypeInfoByDecl("complex<float>");
    EXPECT_NE(ti, nullptr);
    ti = engine->GetTypeInfoByDecl("complex<double>");
    EXPECT_NE(ti, nullptr);
}

template <bool UseGeneric>
class basic_complex_suite : public ::testing::Test
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
                FAIL() << "complex assertion failed: " << msg;
            }
        );

        asbind20::ext::register_math_function(
            m_engine,
            UseGeneric
        );
        asbind20::ext::register_math_complex(
            m_engine,
            UseGeneric
        );
    }

    void TearDown() override
    {
        m_engine.reset();
    }

    auto get_engine() const
        -> AS_NAMESPACE_QUALIFIER asIScriptEngine*
    {
        return m_engine.get();
    }

private:
    asbind20::script_engine m_engine;
};
} // namespace test_ext_math

using ext_complex_native = test_ext_math::basic_complex_suite<false>;
using ext_complex_generic = test_ext_math::basic_complex_suite<true>;

TEST_F(ext_complex_native, checks)
{
    auto* engine = get_engine();

    test_ext_math::check_math_complex(engine);
    test_ext_math::check_complex_template(engine);
}

TEST_F(ext_complex_generic, checks)
{
    auto* engine = get_engine();

    test_ext_math::check_math_complex(engine);
    test_ext_math::check_complex_template(engine);
}

TEST_F(ext_complex_native, template_callback)
{
    auto* engine = get_engine();

    test_ext_math::check_complex_template(engine);
}

TEST_F(ext_complex_generic, template_callback)
{
    auto* engine = get_engine();

    test_ext_math::check_complex_template(engine);
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
