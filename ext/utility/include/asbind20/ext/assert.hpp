#ifndef ASBIND20_EXT_ASSERT_HPP
#define ASBIND20_EXT_ASSERT_HPP

#pragma once

#include <functional>
#include <asbind20/asbind.hpp>

namespace asbind20::ext
{
using assert_handler_type = void(std::string_view);

// TODO: Remove parameter for string factory after upgrading to AngelScript 2.38

/**
 * @brief Register script assertion support
 *
 * @param callback Callback for assertion failure
 * @param set_ex Set script exception on assertion failure
 * @param str_factory String factory for extracting assertion message
 */
void register_script_assert(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    std::function<assert_handler_type> callback,
    bool set_ex = true,
    AS_NAMESPACE_QUALIFIER asIStringFactory* str_factory = nullptr
);
} // namespace asbind20::ext

#endif
