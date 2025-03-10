#ifndef ASBIND20_EXT_MATH_HPP
#define ASBIND20_EXT_MATH_HPP

#pragma once

#include <limits>
#include <cmath>
#include <concepts>
#include <asbind20/asbind.hpp>

namespace asbind20::ext
{
/**
 * @brief Register math constants
 *
 * @param ns Namespace of constants
 */
void register_math_constants(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    const char* ns = "numbers"
);

/**
 * @brief Helper for comparing equality of floating points
 *
 * @param epsilon Epsilon value
 */
template <std::floating_point Float>
constexpr bool math_close_to(
    Float a, Float b, Float epsilon = std::numeric_limits<Float>::epsilon()
) noexcept
{
    return std::abs(a - b) < epsilon;
}

void register_math_function(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    bool generic = has_max_portability()
);

void register_math_complex(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    bool generic = has_max_portability()
);
} // namespace asbind20::ext

#endif
