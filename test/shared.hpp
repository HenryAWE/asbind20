#pragma once

#include <filesystem>
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

    void run_file(
        const std::filesystem::path& filename,
        const char* entry_decl = "void main()"
    );

private:
    asIScriptEngine* m_engine = nullptr;
};
} // namespace asbind_test
