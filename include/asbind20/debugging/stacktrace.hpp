#ifndef ASBIND20_DEBUGGING_STACKTRACE_HPP
#define ASBIND20_DEBUGGING_STACKTRACE_HPP

#pragma once

#include <sstream>
#include <vector>
#include "../fwd.hpp"
#include "../utility.hpp"
#ifdef ASBIND20_HAS_LIB_FORMAT
#    include <format>
#endif

namespace asbind20::debugging
{
class stacktrace_entry;
template <typename Allocator>
class basic_stacktrace;

using stacktrace = basic_stacktrace<std::allocator<stacktrace_entry>>;

class stacktrace_entry
{
    template <typename Allocator>
    friend class basic_stacktrace;

    stacktrace_entry(
        context_pointer ctx,
        AS_NAMESPACE_QUALIFIER asUINT stack_level
    )
    {
        if(!ctx)
            return;
        m_func = ctx->GetFunction(stack_level);
    }

public:
    stacktrace_entry(const stacktrace_entry&) noexcept = default;

    stacktrace_entry& operator=(const stacktrace_entry&) noexcept = default;

    [[nodiscard]]
    function_pointer get_function() const noexcept
    {
        return m_func;
    }

    explicit operator bool() const noexcept
    {
        return m_func != nullptr;
    }

    [[nodiscard]]
    std::string description() const
    {
        // Output nothing if the function is nullptr
        if(!m_func) [[unlikely]]
            return {};

        std::ostringstream ss;
        output_desc(ss);
        return std::move(ss).str();
    }

    friend std::ostream& operator<<(std::ostream& os, const stacktrace_entry& entry)
    {
        // Output nothing if the function is nullptr
        if(entry.m_func)
            entry.output_desc(os);
        return os;
    }

private:
    function_pointer m_func = nullptr;

    void output_desc(std::ostream& os) const
    {
        os << m_func->GetName();

        bool first = true;
        bool ex_info_written = false;
        auto output_helper = [&](const auto& v)
        {
            if(!first)
                os << ':';
            else
            {
                first = false;
                os << ' ' << '(';
            }
            os << v;
            ex_info_written = true;
        };

        const char* mod_name = m_func->GetModuleName();
        if(mod_name)
            output_helper(mod_name);

#ifdef ASBIND20_HAS_SCRIPT_FUNCTION_GET_DECLARED_AT
        const char* section_name = "";
        int row = 0, col = 0;
        int r = m_func->GetDeclaredAt(&section_name, &row, &col);
        if(r >= 0)
        {
            output_helper(section_name);
            output_helper(row);
            output_helper(col);
        }

#else
        const char* section_name = m_func->GetScriptSectionName();
        if(section_name)
            output_helper(section_name);
#endif

        if(ex_info_written)
            os << ')';
    }
};

template <typename Allocator>
class basic_stacktrace
{
    using container_type = std::vector<stacktrace_entry, Allocator>;

public:
    using size_type = AS_NAMESPACE_QUALIFIER asUINT;
    using const_iterator = typename container_type::const_iterator;
    using iterator = const_iterator;

    basic_stacktrace() = default;
    basic_stacktrace(const basic_stacktrace&) = default;
    basic_stacktrace(basic_stacktrace&&) noexcept = default;

    basic_stacktrace& operator=(const basic_stacktrace&) = default;
    basic_stacktrace& operator=(basic_stacktrace&&) noexcept = default;

    static basic_stacktrace from_context(context_pointer ctx)
    {
        basic_stacktrace result;
        if(!ctx)
            return result;

        const size_type max_level = ctx->GetCallstackSize();
        result.m_entries.reserve(max_level);
        for(size_type i = 0; i < max_level; ++i)
        {
            result.m_entries.emplace_back(
                stacktrace_entry(ctx, i)
            );
        }

        return result;
    }

    static basic_stacktrace current()
    {
        return from_context(current_context());
    }

    friend std::ostream& operator<<(std::ostream& os, const basic_stacktrace& trace)
    {
        // Output nothing if the trace is empty
        for(std::size_t i = 0; i < trace.m_entries.size(); ++i)
        {
            os << '#' << i << ' ';
            os << trace.m_entries[i] << '\n';
        }

        return os;
    }

    [[nodiscard]]
    std::string description() const
    {
        std::ostringstream ss;
        ss << *this;
        return std::move(ss).str();
    }

    [[nodiscard]]
    size_type size() const noexcept
    {
        return static_cast<size_type>(m_entries.size());
    }

    [[nodiscard]]
    bool empty() const noexcept
    {
        return m_entries.empty();
    }

    const_iterator cbegin() const noexcept
    {
        return m_entries.cbegin();
    }

    const_iterator cend() const noexcept
    {
        return m_entries.cend();
    }

    const_iterator begin() const noexcept
    {
        return cbegin();
    }

    const_iterator end() const noexcept
    {
        return cend();
    }

private:
    container_type m_entries;
};

inline std::string to_string(const stacktrace_entry& entry)
{
    return entry.description();
}

template <typename Allocator>
std::string to_string(const basic_stacktrace<Allocator>& trace)
{
    return trace.description();
}
} // namespace asbind20::debugging

#ifdef ASBIND20_HAS_LIB_FORMAT

template <>
struct std::formatter<asbind20::debugging::stacktrace_entry> :
    std::formatter<std::string>
{
private:
    using my_base = std::formatter<std::string>;

public:
    auto format(
        const asbind20::debugging::stacktrace_entry& entry,
        std::format_context& ctx
    ) const
    {
        return my_base::format(
            to_string(entry), ctx
        );
    }
};

template <typename Allocator>
struct std::formatter<asbind20::debugging::basic_stacktrace<Allocator>> :
    std::formatter<std::string>
{
private:
    using my_base = std::formatter<std::string>;

public:
    auto format(
        const asbind20::debugging::basic_stacktrace<Allocator>& trace,
        std::format_context& ctx
    ) const
    {
        return my_base::format(
            to_string(trace), ctx
        );
    }
};

#endif

#endif
