// Script assertion for testing

#pragma once

#include <functional>
#include <asbind20/asbind.hpp>

namespace asbind_test
{
extern std::function<void(std::string_view)> global_assertion_callback;

void default_assertion_callback(std::string_view msg);

// Get full assertion message with prefix of context information
std::string gen_full_assert_msg(
    AS_NAMESPACE_QUALIFIER asIScriptContext* ctx, std::string_view msg
);

void setup_script_assertion(
    asbind20::engine_pointer engine
);
} // namespace asbind_test
