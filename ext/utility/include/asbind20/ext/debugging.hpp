/**
 * @file debugging.hpp
 * @author HenryAWE
 * @brief Tools for debugging scripts
 *
 */

#ifndef ASBIND20_EXT_DEBUGGING_HPP
#define ASBIND20_EXT_DEBUGGING_HPP

#pragma once

#include <string>
#include <asbind20/asbind.hpp>

namespace asbind20::ext
{
/**
 * @brief Extracts the contents from a script string without knowing the underlying type
 *
 * @param factory The string factory
 * @param str The pointer to script string
 *
 * @throws std::runtime_error Failed to get the string
 *
 * @return std::string Result string
 */
std::string extract_string(
    AS_NAMESPACE_QUALIFIER asIStringFactory* factory, const void* str
);

// AngelScript 2.38+ supports retrieving string factory from engine
#if ANGELSCRIPT_VERSION >= 23800

inline std::string extract_string(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, const void* str
)
{
    AS_NAMESPACE_QUALIFIER asIStringFactory* factory = nullptr;
    int r = engine->GetStringFactory(nullptr, &factory);
    if(r != AS_NAMESPACE_QUALIFIER asSUCCESS || !factory)
    {
        throw std::runtime_error("failed to get string factory");
    }

    return extract_string(factory, str);
}

#endif
} // namespace asbind20::ext

#endif
