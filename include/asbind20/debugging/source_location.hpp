/**
 * @file source_location.hpp
 * @author HenryAWE
 * @brief Script source location
 */

#ifndef ASBIND20_DEBUGGING_SOURCE_LOCATION_HPP
#define ASBIND20_DEBUGGING_SOURCE_LOCATION_HPP

#pragma once

#include "../fwd.hpp"
#include "../detail/strutil.hpp"

namespace asbind20::debugging
{
using stack_level_type = AS_NAMESPACE_QUALIFIER asUINT;

class script_source_location
{
public:
    using value_type = int;

    constexpr script_source_location() noexcept = default;
    constexpr script_source_location(const script_source_location&) noexcept = default;

    script_source_location& operator=(const script_source_location&) noexcept = default;

    [[nodiscard]]
    cstring_ref section_name() const noexcept
    {
        return m_section_name;
    }

    [[nodiscard]]
    cstring_ref function_name() const noexcept
    {
        return m_function_name;
    }

    [[nodiscard]]
    int line() const noexcept
    {
        return m_line;
    }

    [[nodiscard]]
    int column() const noexcept
    {
        return m_column;
    }

#ifdef ASBIND20_HAS_SCRIPT_FUNCTION_GET_DECLARED_AT

    [[nodiscard]]
    static script_source_location from_function(const_function_reference func)
    {
        script_source_location result{};

        const char* section = nullptr;
        func.GetDeclaredAt(&section, &result.m_line, &result.m_column);
        result.m_section_name = section;
        result.m_function_name = func.GetName();

        return result;
    }

    [[nodiscard]]
    static script_source_location from_function(const_function_pointer func)
    {
        if(!func) [[unlikely]]
            return {};
        return from_function(*func);
    }

#endif
    [[nodiscard]]
    static script_source_location from_context(
        context_reference ctx,
        stack_level_type stack_level = 0
    )
    {
        script_source_location result{};

        const char* section = nullptr;
        result.m_line = ctx.GetLineNumber(stack_level, &result.m_column, &section);
        result.m_section_name = section;
        auto* func = ctx.GetFunction(stack_level);
        if(func)
            result.m_function_name = func->GetName();

        return result;
    }

    [[nodiscard]]
    static script_source_location from_context(
        context_pointer ctx,
        stack_level_type stack_level = 0
    )
    {
        if(!ctx) [[unlikely]]
            return {};
        return from_context(*ctx, stack_level);
    }

private:
    cstring_ref m_section_name{};
    cstring_ref m_function_name{};
    int m_line = 0;
    int m_column = 0;
};
} // namespace asbind20::debugging

#endif
