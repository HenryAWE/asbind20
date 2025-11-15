// Script assertion for testing

#pragma once

#include <functional>
#include <asbind20/asbind.hpp>

namespace asbind_test
{
extern std::function<void(std::string_view)> global_assertion_callback;

void default_assertion_callback(std::string_view msg);

void setup_script_assertion(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine
);
} // namespace asbind_test
