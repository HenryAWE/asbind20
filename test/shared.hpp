#pragma once

#include <filesystem>
#include <gtest/gtest.h>
#include <asbind20/asbind.hpp>

namespace asbind_test
{
template <typename T>
::testing::AssertionResult result_has_value(const asbind20::script_invoke_result<T>& r)
{
    if(r.has_value())
        return ::testing::AssertionSuccess();
    else
    {
        const char* state_str = "";
        switch(r.error())
        {
        case asEXECUTION_FINISHED: state_str = "FINISHED"; break;
        case asEXECUTION_SUSPENDED: state_str = "SUSPENDED"; break;
        case asEXECUTION_ABORTED: state_str = "ABORTED"; break;
        case asEXECUTION_EXCEPTION: state_str = "EXCEPTION"; break;
        case asEXECUTION_PREPARED: state_str = "PREPARED"; break;
        case asEXECUTION_UNINITIALIZED: state_str = "UNINITIALIZED"; break;
        case asEXECUTION_ACTIVE: state_str = "ACTIVE"; break;
        case asEXECUTION_ERROR: state_str = "ERROR"; break;
        case asEXECUTION_DESERIALIZATION: state_str = "DESERIALIZATION"; break;
        }

        return ::testing::AssertionFailure()
               << "r = " << r.error() << ' ' << state_str;
    }
}

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
