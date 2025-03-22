/**
 * @file operators.hpp
 * @author HenryAWE
 * @brief Tools for registering operators
 *
 * For the predefined operator name in AngelScript, see:
 * https://www.angelcode.com/angelscript/sdk/docs/manual/doc_script_class_ops.html
 */

#ifndef ASBIND20_OPERATORS_HPP
#define ASBIND20_OPERATORS_HPP

#pragma once

#include "utility.hpp" // this_type

namespace asbind20
{
template <bool IsConst>
struct this_placeholder
{
    static constexpr this_placeholder<true> as_const() noexcept
    {
        return {};
    }

    static constexpr bool is_const() noexcept
    {
        return IsConst;
    }
};

inline constexpr this_placeholder<false> _this;
inline constexpr this_placeholder<true> const_this;

namespace detail
{
    template <bool IsValueType, bool IsReference>
    std::string full_param_decl(
        bool is_const,
        std::string_view decl,
        AS_NAMESPACE_QUALIFIER asETypeModifiers tm
    )
    {
        if constexpr(!IsReference)
            return std::string(decl);
        else
        {
            assert(tm != AS_NAMESPACE_QUALIFIER asTM_NONE && "Reference modifier is required");

            if constexpr(IsValueType)
            {
                if(is_const || tm == AS_NAMESPACE_QUALIFIER asTM_INREF)
                    return string_concat(decl, "&in");
                else if(tm == AS_NAMESPACE_QUALIFIER asTM_OUTREF)
                    return string_concat(decl, "&out");
                else // tm == asTM_INOUTREF
                    assert(false && "&inout for value type is invalid");
            }
            else
            {
                if(is_const || tm == AS_NAMESPACE_QUALIFIER asTM_INREF)
                    return string_concat(decl, "&in");
                else if(tm == AS_NAMESPACE_QUALIFIER asTM_OUTREF)
                    return string_concat(decl, "&out");
                else // tm == asTM_INOUTREF
                    return string_concat(decl, '&');
            }
        }
    }
} // namespace detail

template <typename T, bool AutoDecl = false>
struct param_t;

template <typename T>
struct param_t<T, false>
{
    constexpr param_t(const param_t&) = default;

    std::string_view declaration;

    [[nodiscard]]
    std::string_view get_decl() const noexcept
    {
        return declaration;
    }
};

template <typename T>
struct param_t<T, true>
{
    constexpr param_t() = default;
    constexpr param_t(const param_t&) = default;

    [[nodiscard]]
    constexpr std::string_view get_decl() const noexcept
        requires(has_static_name<std::remove_cvref_t<T>>)
    {
        return meta::full_fixed_name_of<T>();
    }

    constexpr param_t<T, false> operator()(std::string_view decl) const noexcept
    {
        return param_t<T, false>{decl};
    }
};

template <typename T>
constexpr inline param_t<T, true> param{};

namespace operators
{
    template <typename T>
    concept proxy = requires() {
        typename T::operator_proxy_tag;
    };

    template <typename Lhs, typename Rhs>
    class binary_operator
    {
    public:
        using operator_proxy_tag = void;

        static constexpr std::size_t operand_count = 2;

        using lhs_type = Lhs;
        using rhs_type = Rhs;
    };

    template <typename Lhs, typename Rhs>
    class opAdd;

    template <bool LhsConst, bool RhsConst>
    class opAdd<this_placeholder<LhsConst>, this_placeholder<RhsConst>> : public binary_operator<this_type_t, this_type_t>
    {
    public:
        static constexpr auto name = meta::fixed_string("opAdd");

        template <typename Self>
        using result_type = decltype(std::declval<const Self>() + std::declval<const Self>());
    };

    template <bool LhsConst, typename Rhs, bool AutoDecl>
    class opAdd<this_placeholder<LhsConst>, param_t<Rhs, AutoDecl>> :
        private param_t<Rhs, AutoDecl>,
        public binary_operator<this_type_t, Rhs>
    {
        using param_type = param_t<Rhs, AutoDecl>;

    public:
        static constexpr auto name = meta::fixed_string("opAdd");

        opAdd(const param_type& param)
            : param_type(param) {}

        template <typename Self>
        using result_type = decltype(std::declval<Self>() + std::declval<Rhs>());

        const opAdd* operator->() const noexcept
        {
            return this;
        }

        template <typename Result>
        requires(has_static_name<std::remove_cvref_t<Result>>)
        void return_() const
        {}

        template <this_type_t>
        void return_() const
        {}
    };
} // namespace operators

template <bool LhsConst, typename Rhs, bool AutoDecl>
constexpr auto operator+(this_placeholder<LhsConst>, const param_t<Rhs, AutoDecl>& rhs)
    -> operators::opAdd<this_placeholder<LhsConst>, param_t<Rhs, AutoDecl>>
{
    return {rhs};
}
} // namespace asbind20

#endif
