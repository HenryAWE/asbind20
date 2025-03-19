#ifndef ASBIND20_EXT_MATH_HPP
#define ASBIND20_EXT_MATH_HPP

#pragma once

#include <limits>
#include <cmath>
#include <concepts>
#include <complex>
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
    std::string_view ns = "numbers"
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

template <typename Float>
constexpr Float complex_squared_length(const std::complex<Float>& c) noexcept
{
    return c.real() * c.real() + c.imag() * c.imag();
}

template <typename Float>
constexpr Float complex_length(const std::complex<Float>& c) noexcept
{
    return std::sqrt(complex_squared_length<Float>(c));
}

/**
 * @brief Register `std::complex<float>` and `std::complex<double>`
 */
void register_math_complex(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    bool use_generic = has_max_portability()
);
} // namespace asbind20::ext

#endif
