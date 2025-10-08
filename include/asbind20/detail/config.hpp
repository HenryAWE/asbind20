/**
 * @file detail/config.hpp
 * @author HenryAWE
 * @brief Compile-time config
 */

#ifndef ASBIND20_DETAIL_CONFIG_HPP
#define ASBIND20_DETAIL_CONFIG_HPP

#pragma once

#include <version>
#include "include_as.hpp"

#if defined(_WIN32) && !defined(_WIN64)
#    define ASBIND20_HAS_STANDALONE_STDCALL
#    define ASBIND20_CDECL   __cdecl
#    define ASBIND20_STDCALL __stdcall
#else
// placeholder
#    define ASBIND20_CDECL
#    define ASBIND20_STDCALL
#endif

#if ANGELSCRIPT_VERSION >= 23800
#    define ASBIND20_HAS_AS_FOREACH
#endif

#if ANGELSCRIPT_VERSION >= 23900
#    define ASBIND20_HAS_ENUM_UNDERLYING_TYPE
#endif

#endif
