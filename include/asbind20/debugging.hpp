/**
 * @file debugging.hpp
 * @author HenryAWE
 * @brief Tools for debugging
 */

#ifndef ASBIND20_DEBUGGING_HPP
#define ASBIND20_DEBUGGING_HPP

#pragma once

#include "fwd.hpp"

// IWYU pragma: begin_exports

#include "debugging/extract_string.hpp"
#include "debugging/gc_statistics.hpp"
#include "debugging/source_location.hpp"

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
    const_function_pointer func
)
{
    if(!func) [[unlikely]]
        return nullptr;

#ifdef ASBIND20_HAS_SCRIPT_FUNCTION_GET_DECLARED_AT
    const char* result = nullptr;
    func->GetDeclaredAt(&result, nullptr, nullptr);
    return result;

#else
    return func->GetScriptSectionName();
#endif
}
} // namespace asbind20::debugging

#endif
