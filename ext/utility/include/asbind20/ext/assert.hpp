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
std::string extract_string(asIStringFactory* factory, void* str);

using assert_handler_t = void(std::string_view);

std::function<assert_handler_t> register_script_assert(
    asIScriptEngine* engine,
    std::function<assert_handler_t> callback,
    bool set_ex = true,
    asIStringFactory* str_factory = nullptr
);
} // namespace asbind20::ext

#endif
