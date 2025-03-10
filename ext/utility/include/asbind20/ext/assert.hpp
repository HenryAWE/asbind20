#ifndef ASBIND20_EXT_ASSERT_HPP
#define ASBIND20_EXT_ASSERT_HPP

#pragma once

#include <functional>
#include <asbind20/asbind.hpp>

namespace asbind20::ext
{
/**
 * @brief Extracts the contents from a script string without knowing the underlying type
 *
 * @param factory The string factory
 * @param str The pointer to script string
 *
 * @return std::string Result string
 */
std::string extract_string(
    AS_NAMESPACE_QUALIFIER asIStringFactory* factory, const void* str
);

using assert_handler_t = void(std::string_view);

/**
 * @brief Register script assertion support
 *
 * @param callback Callback for assertion failure
 * @param set_ex Set script exception on assertion failure
 * @param str_factory String factory for extracting assertion message
 */
void register_script_assert(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    std::function<assert_handler_t> callback,
    bool set_ex = true,
    AS_NAMESPACE_QUALIFIER asIStringFactory* str_factory = nullptr
);
} // namespace asbind20::ext

#endif
