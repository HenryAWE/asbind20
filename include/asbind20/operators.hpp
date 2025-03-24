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
    [[nodiscard]]
    static constexpr this_placeholder<true> as_const() noexcept
    {
        return {};
    }

    [[nodiscard]]
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
struct param_placeholder;

template <typename T>
struct param_placeholder<T, false>
{
    constexpr param_placeholder(const param_placeholder&) = default;

    constexpr param_placeholder(std::string_view decl) noexcept
        : declaration(decl) {}

    std::string_view declaration;

    [[nodiscard]]
    std::string_view get_decl() const noexcept
    {
        return declaration;
    }
};

template <typename T>
struct param_placeholder<T, true>
{
    constexpr param_placeholder() = default;
    constexpr param_placeholder(const param_placeholder&) = default;

    [[nodiscard]]
    constexpr auto get_decl() const noexcept
        requires(has_static_name<std::remove_cvref_t<T>>)
    {
        return meta::full_fixed_name_of<T>();
    }

    [[nodiscard]]
    constexpr param_placeholder<T, false> operator()(std::string_view decl) const noexcept
    {
        return param_placeholder<T, false>{decl};
    }
};

template <typename T>
constexpr inline param_placeholder<T, true> param{};

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
} // namespace operators

namespace operators
{
    template <typename Lhs, typename Rhs>
    class opAdd;

    template <bool LhsConst, bool RhsConst>
    class opAdd<this_placeholder<LhsConst>, this_placeholder<RhsConst>> : public binary_operator<this_type_t, this_type_t>
    {
    public:
        static constexpr auto name = meta::fixed_string("opAdd");

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
                using rhs_arg_type = std::conditional_t<
                    RhsConst,
                    std::add_const_t<class_type>,
                    class_type>;

                auto rhs_decl = ar.get_name();

                std::string decl = string_concat(
                    detail::get_return_decl<Return>(ar),
                    ' ',
                    name,
                    '(',
                    RhsConst ? "const " : "",
                    rhs_decl,
                    RhsConst ? "&in" : "&",
                    ')',
                    LhsConst ? "const" : ""
                );

                ar.method(
                    decl,
                    [](this_arg_type& lhs, rhs_arg_type& rhs) -> Return
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
        class return_proxy_with_decl
        {
        public:
            return_proxy_with_decl(const opAdd& proxy, std::string_view ret_decl)
                : m_proxy(&proxy), m_ret_decl(ret_decl) {}

            template <typename RegisterHelper>
            void operator()(RegisterHelper& ar) const
            {
                using class_type = typename RegisterHelper::class_type;
                using this_arg_type = std::conditional_t<
                    LhsConst,
                    std::add_const_t<class_type>,
                    class_type>;
                using rhs_arg_type = std::conditional_t<
                    RhsConst,
                    std::add_const_t<class_type>,
                    class_type>;

                auto rhs_decl = ar.get_name();

                std::string decl = string_concat(
                    m_ret_decl,
                    ' ',
                    name,
                    '(',
                    RhsConst ? "const " : "",
                    rhs_decl,
                    RhsConst ? "&in" : "&",
                    ')',
                    LhsConst ? "const" : ""
                );

                ar.method(
                    decl,
                    [](this_arg_type& lhs, rhs_arg_type& rhs) -> Return
                    {
                        return lhs + rhs;
                    },
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>
                );
            }

        private:
            const opAdd* m_proxy;
            std::string_view m_ret_decl;
        };

        template <typename Return>
        requires(has_static_name<std::remove_cvref_t<Return>>)
        [[nodiscard]]
        return_proxy<Return> return_() const
        {
            return *this;
        }

        template <typename Return>
        [[nodiscard]]
        return_proxy_with_decl<Return> return_(std::string_view ret_decl) const
        {
            return {*this, ret_decl};
        }
    };

    template <bool LhsConst, typename Rhs, bool AutoDecl>
    class opAdd<this_placeholder<LhsConst>, param_placeholder<Rhs, AutoDecl>> :
        private param_placeholder<Rhs, AutoDecl>,
        public binary_operator<this_type_t, Rhs>
    {
        using param_type = param_placeholder<Rhs, AutoDecl>;

    public:
        static constexpr auto name = meta::fixed_string("opAdd");

        opAdd(const param_type& param)
            : param_type(param) {}

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

                auto rhs_decl = m_proxy->param_type::get_decl();

                std::string decl = string_concat(
                    detail::get_return_decl<Return>(ar),
                    ' ',
                    name,
                    '(',
                    rhs_decl,
                    ')',
                    LhsConst ? "const" : ""
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
        class return_proxy_with_decl
        {
        public:
            return_proxy_with_decl(const opAdd& proxy, std::string_view ret_decl)
                : m_proxy(&proxy), m_ret_decl(ret_decl) {}

            template <typename RegisterHelper>
            void operator()(RegisterHelper& ar) const
            {
                using class_type = typename RegisterHelper::class_type;
                using this_arg_type = std::conditional_t<
                    LhsConst,
                    std::add_const_t<class_type>,
                    class_type>;

                auto rhs_decl = m_proxy->param_type::get_decl();

                std::string decl = string_concat(
                    m_ret_decl,
                    ' ',
                    name,
                    '(',
                    rhs_decl,
                    ')',
                    LhsConst ? "const" : ""
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
            std::string_view m_ret_decl;
        };

        template <typename Return>
        requires(has_static_name<std::remove_cvref_t<Return>>)
        [[nodiscard]]
        return_proxy<Return> return_() const
        {
            return *this;
        }

        template <typename Return>
        [[nodiscard]]
        return_proxy_with_decl<Return> return_(std::string_view ret_decl) const
        {
            return {*this, ret_decl};
        }
    };

    template <typename Lhs, bool AutoDecl, bool RhsConst>
    class opAdd<param_placeholder<Lhs, AutoDecl>, this_placeholder<RhsConst>> :
        private param_placeholder<Lhs, AutoDecl>,
        public binary_operator<this_type_t, Lhs>
    {
        using param_type = param_placeholder<Lhs, AutoDecl>;

    public:
        static constexpr auto name = meta::fixed_string("opAdd_r");

        opAdd(const param_type& param)
            : param_type(param) {}

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
                    RhsConst,
                    std::add_const_t<class_type>,
                    class_type>;

                auto lhs_decl = m_proxy->param_type::get_decl();

                std::string decl = string_concat(
                    detail::get_return_decl<Return>(ar),
                    ' ',
                    name,
                    '(',
                    lhs_decl,
                    ')',
                    RhsConst ? "const" : ""
                );

                ar.method(
                    decl,
                    [](Lhs lhs, this_arg_type& rhs) -> Return
                    {
                        return lhs + rhs;
                    },
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>
                );
            }

        private:
            const opAdd* m_proxy;
        };

        template <typename Return>
        class return_proxy_with_decl
        {
        public:
            return_proxy_with_decl(const opAdd& proxy, std::string_view ret_decl)
                : m_proxy(&proxy), m_ret_decl(ret_decl) {}

            template <typename RegisterHelper>
            void operator()(RegisterHelper& ar) const
            {
                using class_type = typename RegisterHelper::class_type;
                using this_arg_type = std::conditional_t<
                    RhsConst,
                    std::add_const_t<class_type>,
                    class_type>;

                auto lhs_decl = m_proxy->param_type::get_decl();

                std::string decl = string_concat(
                    m_ret_decl,
                    ' ',
                    name,
                    '(',
                    lhs_decl,
                    ')',
                    RhsConst ? "const" : ""
                );

                ar.method(
                    decl,
                    [](Lhs lhs, this_arg_type& rhs) -> Return
                    {
                        return lhs + rhs;
                    },
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>
                );
            }

        private:
            const opAdd* m_proxy;
            std::string_view m_ret_decl;
        };

        template <typename Return>
        requires(has_static_name<std::remove_cvref_t<Return>>)
        [[nodiscard]]
        return_proxy<Return> return_() const
        {
            return *this;
        }

        template <typename Return>
        [[nodiscard]]
        return_proxy_with_decl<Return> return_(std::string_view ret_decl) const
        {
            return {*this, ret_decl};
        }
    };
} // namespace operators

template <bool LhsConst, bool RhsConst>
constexpr auto operator+(this_placeholder<LhsConst>, this_placeholder<RhsConst>)
    -> operators::opAdd<this_placeholder<LhsConst>, this_placeholder<RhsConst>>
{
    return {};
}

template <bool LhsConst, typename Rhs, bool AutoDecl>
constexpr auto operator+(this_placeholder<LhsConst>, const param_placeholder<Rhs, AutoDecl>& rhs)
    -> operators::opAdd<this_placeholder<LhsConst>, param_placeholder<Rhs, AutoDecl>>
{
    return {rhs};
}

template <typename Lhs, bool AutoDecl, bool RhsConst>
constexpr auto operator+(const param_placeholder<Lhs, AutoDecl>& lhs, this_placeholder<RhsConst>)
    -> operators::opAdd<param_placeholder<Lhs, AutoDecl>, this_placeholder<RhsConst>>
{
    return {lhs};
}
} // namespace asbind20

#endif
