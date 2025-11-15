#pragma once

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

void setup_exception_translator(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine
);

void output_gc_statistics(
    std::ostream& os,
    const asbind20::debugging::gc_statistics& stat,
    char sep = '\n'
);
void output_gc_statistics(
    std::ostream& os,
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    char sep = '\n'
);
} // namespace asbind_test

#define ASBIND_TEST_SKIP_IF_MAX_PORTABILITY()  \
    do {                                       \
        if(::asbind20::has_max_portability())  \
            GTEST_SKIP() << "MAX_PORTABILITY"; \
    } while(0)
