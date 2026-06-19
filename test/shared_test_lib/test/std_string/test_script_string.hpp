#pragma once

#include <asbind_test/framework.hpp>
#include <asbind_test/std_string.hpp>

namespace test_script_string
{
template <bool UseGeneric>
class script_string_suite_base : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_engine = asbind20::make_script_engine();

        if constexpr(!UseGeneric)
            ASBIND_TEST_SKIP_IF_MAX_PORTABILITY();

        asbind_test::setup_message_callback(m_engine, true);
        asbind_test::setup_exception_translator(m_engine);

        asbind_test::configure_engine_for_ext_string(m_engine);
        asbind_test::setup_script_char(m_engine, UseGeneric);
        asbind_test::setup_script_string(m_engine, UseGeneric, true);
        asbind_test::setup_string_utils(m_engine, UseGeneric);
    }

    static auto& get_str_factory()
    {
        return asbind_test::string_factory::get();
    }

    auto get_engine() const
        -> AS_NAMESPACE_QUALIFIER asIScriptEngine*
    {
        return m_engine.get();
    }

    void TearDown() override
    {
        m_engine.reset();
    }

private:
    asbind20::script_engine m_engine;
};
} // namespace test_script_string

