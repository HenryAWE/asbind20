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
        case AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED: state_str = "FINISHED"; break;
        case AS_NAMESPACE_QUALIFIER asEXECUTION_SUSPENDED: state_str = "SUSPENDED"; break;
        case AS_NAMESPACE_QUALIFIER asEXECUTION_ABORTED: state_str = "ABORTED"; break;
        case AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION: state_str = "EXCEPTION"; break;
        case AS_NAMESPACE_QUALIFIER asEXECUTION_PREPARED: state_str = "PREPARED"; break;
        case AS_NAMESPACE_QUALIFIER asEXECUTION_UNINITIALIZED: state_str = "UNINITIALIZED"; break;
        case AS_NAMESPACE_QUALIFIER asEXECUTION_ACTIVE: state_str = "ACTIVE"; break;
        case AS_NAMESPACE_QUALIFIER asEXECUTION_ERROR: state_str = "ERROR"; break;
        case AS_NAMESPACE_QUALIFIER asEXECUTION_DESERIALIZATION: state_str = "DESERIALIZATION"; break;
        }

        return ::testing::AssertionFailure()
               << "r = " << r.error() << ' ' << state_str;
    }
}

void setup_message_callback(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    bool propagate_error_to_gtest = false
);

class asbind_test_suite : public ::testing::Test
{
public:
    void msg_callback(const AS_NAMESPACE_QUALIFIER asSMessageInfo* msg);
    void ex_translator(AS_NAMESPACE_QUALIFIER asIScriptContext* ctx);

    void SetUp() override;

    void TearDown() override;

    AS_NAMESPACE_QUALIFIER asIScriptEngine* get_engine() const noexcept
    {
        return m_engine.get();
    }

    void run_file(
        const std::filesystem::path& filename,
        const char* entry_decl = "void main()"
    );

protected:
    virtual void register_all();

private:
    asbind20::script_engine m_engine;
};

class asbind_test_suite_generic : public asbind_test_suite
{
protected:
    void register_all() override;
};
} // namespace asbind_test
