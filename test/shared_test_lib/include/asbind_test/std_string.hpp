// Register std::string as script string for building test script

#pragma once

#include <string>
#include <asbind20/asbind.hpp>

namespace asbind_test
{


void setup_simple_std_string(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine);
}
