/**
 * @file compare.hpp
 * @author HenryAWE
 * @brief Utilities for comparing element of container
 *
 * @note The content of this file is provided for library developer.
 *       If you're not developing some highly customized container for AngelScript,
 *       maybe the following APIs are not designed for you!
 */

#ifndef ASBIND20_CONTAINER_COMPARE_HPP
#define ASBIND20_CONTAINER_COMPARE_HPP

#pragma once

#include "options.hpp"
#include "../utility.hpp"
#include "../invoke.hpp"
#include "../ranges/ranges.hpp"

namespace asbind20::container
{
class get_comparator_result;

[[nodiscard]]
get_comparator_result get_comparator(
    typeinfo_pointer ti
);

class script_element_comparator
{
public:
    using opEquals_type = script_method<bool(const void*)>;
    using opCmp_type = script_method<int(const void*)>;

    script_element_comparator() = default;
    script_element_comparator(const script_element_comparator&) = default;

    script_element_comparator& operator=(const script_element_comparator&) = default;

    script_element_comparator(
        AS_NAMESPACE_QUALIFIER asIScriptFunction* opCmp,
        AS_NAMESPACE_QUALIFIER asIScriptFunction* opEquals
    )
        : m_opCmp(opCmp), m_opEquals(opEquals)
    {}

    [[nodiscard]]
    const opEquals_type& get_opEquals() const noexcept
    {
        return m_opEquals;
    }

    [[nodiscard]]
    bool has_opEquals() const noexcept
    {
        return static_cast<bool>(m_opEquals);
    }

    [[nodiscard]]
    const opCmp_type& get_opCmp() const noexcept
    {
        return m_opCmp;
    }

    [[nodiscard]]
    bool has_opCmp() const noexcept
    {
        return static_cast<bool>(m_opCmp);
    }

    auto invoke_eq(
        AS_NAMESPACE_QUALIFIER asIScriptContext* ctx, const void* lhs, const void* rhs
    ) const
        -> script_invoke_result<bool>
    {
        return get_opEquals()(ctx, lhs, rhs);
    }

    auto invoke_compare(
        AS_NAMESPACE_QUALIFIER asIScriptContext* ctx, const void* lhs, const void* rhs
    ) const
        -> script_invoke_result<int>
    {
        return get_opCmp()(ctx, lhs, rhs);
    }

    /**
     * @brief Comparing two script objects
     *
     * @param ctx Script context
     * @param lhs Left object
     * @param rhs Right object
     * @return std::partial_ordering Object relationship, or `partial_ordering::unordered` when error occurred.
     */
    [[nodiscard]]
    std::partial_ordering compare(
        AS_NAMESPACE_QUALIFIER asIScriptContext* ctx, const void* lhs, const void* rhs
    ) const
    {
        if(!has_opCmp()) [[unlikely]]
            return std::partial_ordering::unordered;
        auto result = invoke_compare(ctx, lhs, rhs);
        if(!result.has_value())
            return std::partial_ordering::unordered;
        return *result <=> 0;
    }

    [[nodiscard]]
    bool eq(
        AS_NAMESPACE_QUALIFIER asIScriptContext* ctx, const void* lhs, const void* rhs
    ) const
    {
        if(!m_opEquals) // Fallback to the logic of "opCmp() == 0"
            return compare(ctx, lhs, rhs) == 0;

        auto result = m_opEquals(ctx, lhs, rhs);
        return result.value_or(false);
    }

    explicit operator bool() const noexcept
    {
        return has_opCmp() || has_opEquals();
    }

private:
    opCmp_type m_opCmp;
    opEquals_type m_opEquals;
};

class get_comparator_result
{
    friend get_comparator_result get_comparator(
        typeinfo_pointer ti
    );

public:
    get_comparator_result() = delete;
    get_comparator_result(const get_comparator_result&) = default;

    ~get_comparator_result() = default;

    get_comparator_result& operator=(const get_comparator_result&) = default;

    struct status
    {
        using error_type = AS_NAMESPACE_QUALIFIER asERetCodes;

        error_type opEquals = AS_NAMESPACE_QUALIFIER asNO_FUNCTION;
        error_type opCmp = AS_NAMESPACE_QUALIFIER asNO_FUNCTION;

        [[nodiscard]]
        bool good() const noexcept
        {
            return opEquals >= 0 || opCmp >= 0;
        }

        explicit operator bool() const noexcept
        {
            return good();
        }
    };

    using error_type = status::error_type;

    [[nodiscard]]
    status get_status() const noexcept
    {
        return m_status;
    }

    [[nodiscard]]
    script_element_comparator get() const
    {
        return {m_opCmp, m_opEquals};
    }

    explicit operator bool() const noexcept
    {
        return static_cast<bool>(m_status);
    }

    explicit operator script_element_comparator() const
    {
        return get();
    }

private:
    status m_status;
    AS_NAMESPACE_QUALIFIER asIScriptFunction* m_opCmp = nullptr;
    AS_NAMESPACE_QUALIFIER asIScriptFunction* m_opEquals = nullptr;

    get_comparator_result(
        status st,
        AS_NAMESPACE_QUALIFIER asIScriptFunction* opCmp,
        AS_NAMESPACE_QUALIFIER asIScriptFunction* opEquals
    )
        : m_status(st), m_opCmp(opCmp), m_opEquals(opEquals)
    {}
};

[[nodiscard]]
inline get_comparator_result get_comparator(
    typeinfo_pointer ti
)
{
    using namespace std::string_view_literals;

    get_comparator_result::status st{};
    AS_NAMESPACE_QUALIFIER asIScriptFunction* opCmp = nullptr;
    AS_NAMESPACE_QUALIFIER asIScriptFunction* opEquals = nullptr;

    for(auto* f : views::all_methods(ti))
    {
        const char* name = f->GetName();
        AS_NAMESPACE_QUALIFIER asDWORD flags = 0;
        int return_type_id = f->GetReturnTypeId(&flags);
        if(flags != AS_NAMESPACE_QUALIFIER asTM_NONE)
            continue;

        bool is_opCmp = false;
        bool is_opEquals = false;
        if(return_type_id == AS_NAMESPACE_QUALIFIER asTYPEID_INT32 && name == "opCmp"sv)
        {
            is_opCmp = true;
        }
        if(return_type_id == AS_NAMESPACE_QUALIFIER asTYPEID_BOOL && name == "opEquals"sv)
        {
            is_opEquals = true;
        }

        if(is_opCmp)
        {
            if(opCmp)
            {
                st.opCmp = AS_NAMESPACE_QUALIFIER asMULTIPLE_FUNCTIONS;
                opCmp = nullptr;
            }
            else
            {
                st.opCmp = AS_NAMESPACE_QUALIFIER asSUCCESS;
                opCmp = f;
            }
        }

        if(is_opEquals)
        {
            if(opEquals)
            {
                st.opEquals = AS_NAMESPACE_QUALIFIER asMULTIPLE_FUNCTIONS;
                opEquals = nullptr;
            }
            else
            {
                st.opEquals = AS_NAMESPACE_QUALIFIER asSUCCESS;
                opEquals = f;
            }
        }
    }

    return {st, opCmp, opEquals};
}
} // namespace asbind20::container

#endif
