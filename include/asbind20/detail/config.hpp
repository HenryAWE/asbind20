/**
 * @file detail/config.hpp
 * @author HenryAWE
 * @brief Compile-time config
 */

#ifndef ASBIND20_DETAIL_CONFIG_HPP
#define ASBIND20_DETAIL_CONFIG_HPP

#pragma once

#include <cassert>
#include <version>
#include "include_as.hpp"

#ifndef __cpp_exceptions
#    ifndef ASBIND20_NO_EXCEPTIONS
#        define ASBIND20_NO_EXCEPTIONS 1
#    endif
#endif

#ifndef ASBIND20_ASSERT
#    define ASBIND20_ASSERT(x) assert(x)
#endif

#if defined(_WIN32) && !defined(_WIN64)
#    define ASBIND20_HAS_STANDALONE_STDCALL
#    define ASBIND20_CDECL   __cdecl
#    define ASBIND20_STDCALL __stdcall
#else
// placeholder
#    define ASBIND20_CDECL
#    define ASBIND20_STDCALL
#endif

#ifdef __cpp_lib_format
#    define ASBIND20_HAS_LIB_FORMAT __cpp_lib_format
#endif

#if ANGELSCRIPT_VERSION >= 23800
#    define ASBIND20_HAS_AS_FOREACH
#endif

#if ANGELSCRIPT_VERSION >= 23900
#    define ASBIND20_HAS_ENUM_UNDERLYING_TYPE
#endif

#if ANGELSCRIPT_VERSION >= 23900
#    define ASBIND20_HAS_THISCALL_OBJ_FOR_REF_BEHAVIOURS
#endif

#endif
