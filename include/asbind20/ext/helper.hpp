#ifndef ASBIND20_EXT_HELPER_HPP
#define ASBIND20_EXT_HELPER_HPP

#pragma once

#include "../asbind.hpp"

namespace asbind20::ext
{
/**
 * @brief Register hash support for primitive types
 */
void register_script_hash(asIScriptEngine* engine, bool generic = has_max_portability());
} // namespace asbind20::ext

#endif
