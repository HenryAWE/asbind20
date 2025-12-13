/**
 * @file debugging.hpp
 * @author HenryAWE
 * @brief Tools for debugging
 */

#ifndef ASBIND20_DEBUGGING_HPP
#define ASBIND20_DEBUGGING_HPP

#pragma once

#include <cassert>
#include "detail/include_as.hpp"

// IWYU pragma: begin_exports

#include "debugging/extract_string.hpp"
#include "debugging/gc_statistics.hpp"

// IWYU pragma: end_exports

/**
 * @brief Tools for debugging
 */
namespace asbind20::debugging
{
/**
 * @brief Get script section name of function
 *
 * This helper can handle the different interfaces for getting the section name across AngelScript versions.
 *
 * @param func Script function. It cannot be nullptr.
 */
[[nodiscard]]
inline const char* get_function_section_name(
    const AS_NAMESPACE_QUALIFIER asIScriptFunction* func
)
{
    assert(func != nullptr);

#if ANGELSCRIPT_VERSION >= 23800

    const char* result;
    func->GetDeclaredAt(&result, nullptr, nullptr);
    return result;

#else // 2.37
    return func->GetScriptSectionName();
#endif
}
} // namespace asbind20::debugging

#endif
