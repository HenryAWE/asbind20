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

#if ANGELSCRIPT_VERSION >= 23800

    const char* result;
    func->GetDeclaredAt(&result, nullptr, nullptr);
    return result;

#else // 2.37
    return func->GetScriptSectionName();
#endif
}

struct script_source_location
{
    using value_type = int;

    cstring_ref section_name{};
    cstring_ref function_name{};
    int line = 0;
    int column = 0;

    constexpr script_source_location() noexcept = default;
    constexpr script_source_location(const script_source_location&) noexcept = default;

    script_source_location& operator=(const script_source_location&) noexcept = default;

#if ANGELSCRIPT_VERSION >= 23800

    static script_source_location from_function(const_function_pointer func)
    {
        script_source_location result{};
        if(!func) [[unlikely]]
            return result;

        const char* section = nullptr;
        func->GetDeclaredAt(&section, &result.line, &result.column);
        result.section_name = section;
        result.function_name = func->GetName();

        return result;
    }

#endif

    static script_source_location from_context(
        context_pointer ctx,
        AS_NAMESPACE_QUALIFIER asUINT stack_level = 0
    )
    {
        script_source_location result{};
        if(!ctx) [[unlikely]]
            return result;

        const char* section = nullptr;
        result.line = ctx->GetLineNumber(stack_level, &result.column, &section);
        result.section_name = section;
        auto* func = ctx->GetFunction(stack_level);
        if(func)
            result.function_name = func->GetName();

        return result;
    }
};
} // namespace asbind20::debugging

#endif
