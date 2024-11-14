#ifndef ASBIND20_EXT_MATH_HPP
#define ASBIND20_EXT_MATH_HPP

#pragma once

#include <cmath>
#include <concepts>
#include <asbind20/asbind.hpp>

namespace asbind20::ext
{
void register_math_constants(asIScriptEngine* engine, const char* ns = "numbers");

void register_math_function(asIScriptEngine* engine, bool generic = has_max_portability());

template <std::floating_point T>
constexpr bool close_to(T a, T b, T epsilon)
{
    return std::abs(a - b) < epsilon;
}

void register_math_complex(asIScriptEngine* engine, bool generic = has_max_portability());
} // namespace asbind20::ext

#endif
