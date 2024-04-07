#pragma once

#include <angelscript.h>
#include <gtest/gtest.h>

namespace asbind_test
{
class asbind_test_suite : public ::testing::Test
{
public:
    void SetUp() override;

    void TearDown() override;

    asIScriptEngine* get_engine() const noexcept
    {
        return m_engine;
    }

private:
    asIScriptEngine* m_engine = nullptr;
};
} // namespace asbind_test
