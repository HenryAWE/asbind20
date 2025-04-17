#pragma once

#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/assert.hpp>
#include <asbind20/ext/array.hpp>
#include <asbind20/ext/stdstring.hpp>

namespace test_ext_array
{
constexpr char helper_module_name[] = "test_ext_array";

constexpr char helper_module_script[] = R"AngelScript(class my_pair
{
    int x;
    int y;

    my_pair()
    {
        x = 0;
        y = 0;
    }

    my_pair(int x, int y)
    {
        this.x = x;
        this.y = y;
    }

    bool opEquals(const my_pair&in other) const
    {
        return this.x == other.x && this.y == other.y;
    }
};
)AngelScript";

template <bool UseGeneric>
class basic_ext_array_suite : public ::testing::Test
{
public:
    void SetUp() override
    {
        using namespace asbind20;

        if constexpr(!UseGeneric)
        {
            if(has_max_portability())
                GTEST_SKIP() << "AS_MAX_PORTABILITY";
        }

        m_engine = make_script_engine();

        asbind_test::setup_message_callback(m_engine, true);
        asbind_test::setup_exception_translator(m_engine);
        asbind_test::register_instantly_throw<UseGeneric>(m_engine);
        asbind_test::register_throw_on_copy<UseGeneric>(m_engine);

        ext::register_script_assert(
            m_engine,
            [](std::string_view msg)
            {
                auto* ctx = current_context();
                if(ctx)
                {
                    FAIL() << "array assertion failed in \"" << ctx->GetFunction()->GetScriptSectionName() << "\": " << msg;
                }
                else
                {
                    FAIL() << "array assertion failed: " << msg;
                }
            }
        );
        ext::register_script_array(m_engine, true, UseGeneric);
        ext::configure_engine_for_ext_string(m_engine);
        ext::register_script_char(m_engine, UseGeneric);
        ext::register_std_string(m_engine, true, UseGeneric);

        build_helper_module();
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

    void build_helper_module()
    {
        auto* m = m_engine->GetModule(
            helper_module_name, AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
        );
        ASSERT_NE(m, nullptr);
        m->AddScriptSection(
            "test_ext_array_helper_module",
            helper_module_script
        );
        EXPECT_GE(m->Build(), 0);
    }
};

void run_string(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    const char* section,
    std::string_view code
);
} // namespace test_ext_array

using ext_array_native = test_ext_array::basic_ext_array_suite<false>;
using ext_array_generic = test_ext_array::basic_ext_array_suite<true>;
