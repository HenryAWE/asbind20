#ifndef ASBIND20_DEBUGGING_STACKTRACE_HPP
#define ASBIND20_DEBUGGING_STACKTRACE_HPP

#include <sstream>
#include <vector>
#include "../fwd.hpp"
#include "../utility.hpp"
#ifdef ASBIND20_HAS_LIB_FORMAT
#    include <format>
#endif

namespace asbind20::debugging
{
class stacktrace;

class stacktrace_entry
{
    friend class stacktrace;

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

        std::stringstream ss;
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

class stacktrace
{
public:
    using size_type = AS_NAMESPACE_QUALIFIER asUINT;

    stacktrace() = default;
    stacktrace(const stacktrace&) = default;
    stacktrace(stacktrace&&) noexcept = default;

    stacktrace& operator=(const stacktrace&) = default;
    stacktrace& operator=(stacktrace&&) noexcept = default;

    static stacktrace from_context(context_pointer ctx)
    {
        stacktrace result;
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

    static stacktrace current()
    {
        return from_context(current_context());
    }

    friend std::ostream& operator<<(std::ostream& os, const stacktrace& trace)
    {
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
        std::stringstream ss;
        ss << *this;
        return std::move(ss).str();
    }

private:
    std::vector<stacktrace_entry> m_entries;
};

inline std::string to_string(const stacktrace_entry& entry)
{
    return entry.description();
}

inline std::string to_string(const stacktrace& trace)
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

template <>
struct std::formatter<asbind20::debugging::stacktrace> :
    std::formatter<std::string>
{
private:
    using my_base = std::formatter<std::string>;

public:
    auto format(
        const asbind20::debugging::stacktrace& trace,
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