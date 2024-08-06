#ifndef ASBIND20_EXT_MATH_HPP
#define ASBIND20_EXT_MATH_HPP

#pragma once

#include <cmath>
#include "../asbind.hpp"

namespace asbind20::ext
{
void register_math_function(asIScriptEngine* engine, bool disable_double = true);

template <std::floating_point T>
bool script_close_to(T lhs, T rhs, T epsilon)
{
    return std::abs(lhs - rhs) < epsilon;
}
} // namespace asbind20::ext

#endif
