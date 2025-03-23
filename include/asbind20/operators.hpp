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

#include "generic.hpp"
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
    constexpr auto get_decl() const noexcept
        requires(has_static_name<std::remove_cvref_t<T>>)
    {
        return meta::full_fixed_name_of<true, T, asETypeModifiers::asTM_INREF>();
    }

    constexpr param_t<T, false> operator()(std::string_view decl) const noexcept
    {
        return param_t<T, false>{decl};
    }
};

template <typename T>
constexpr inline param_t<T, true> param{};

namespace detail
{
    template <typename T, typename RegisterHelper>
    std::string get_return_decl(const RegisterHelper& c)
    {
        constexpr bool is_this_type =
            std::same_as<std::remove_cvref_t<T>, typename RegisterHelper::class_type>;

        constexpr bool is_const = std::is_const_v<std::remove_reference_t<T>>;
        constexpr bool is_ref = std::is_reference_v<T>;

        auto modifier_helper = [](auto type_decl) -> std::string
        {
            if constexpr(is_const && is_ref)
                return string_concat("const ", type_decl, '&');
            else if constexpr(is_ref)
                return string_concat(type_decl, '&');
            else
                return std::string(type_decl);
        };

        if constexpr(is_this_type)
        {
            return modifier_helper(c.get_name());
        }
        else if constexpr(has_static_name<std::remove_cvref_t<T>>)
        {
            return modifier_helper(name_of<std::remove_cvref_t<T>>().view());
        }
        else
        {
            static_assert(!sizeof(T), "Cannot deduce declaration of return type");
        }
    }
} // namespace detail

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

        template <typename Return>
        class return_proxy
        {
        public:
            return_proxy(const opAdd& proxy)
                : m_proxy(&proxy) {}

            template <typename RegisterHelper>
            void operator()(RegisterHelper& ar) const
            {
                using class_type = typename RegisterHelper::class_type;
                using this_arg_type = std::conditional_t<
                    LhsConst,
                    std::add_const_t<class_type>,
                    class_type>;

                std::string lhs_decl;
                if constexpr(LhsConst)
                    lhs_decl = string_concat("const ", ar.get_name(), "&in");
                else
                    lhs_decl = string_concat(ar.get_name(), '&');

                auto rhs_decl = m_proxy->param_type::get_decl();

                std::string decl = string_concat(
                    detail::get_return_decl<Return>(ar),
                    ' ',
                    name,
                    '(',
                    lhs_decl,
                    ',',
                    rhs_decl,
                    ')'
                );

                ar.method(
                    decl,
                    [](this_arg_type& lhs, Rhs rhs) -> Return
                    {
                        return lhs + rhs;
                    },
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>
                );
            }

        private:
            const opAdd* m_proxy;
        };

        template <typename Return>
        requires(has_static_name<std::remove_cvref_t<Return>>)
        return_proxy<Return> return_() const
        {
            return *this;
        }

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
