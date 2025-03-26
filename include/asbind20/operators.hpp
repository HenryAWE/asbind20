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

template <bool IsConst>
struct this_placeholder;

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

namespace operators
{
    class binary_operator
    {
    public:
        static constexpr std::size_t operand_count = 2;

    protected:
        // The parameter needs to generate full declaration from a plain type name
        template <bool ParamRef, bool ParamConst>
        constexpr static std::string gen_name_for_auto_decl(
            std::string_view ret_decl,
            std::string_view op_name,
            std::string_view param_name,
            bool is_const
        )
        {
            if constexpr(ParamConst && ParamRef)
            {
                return string_concat(
                    ret_decl,
                    ' ',
                    op_name,
                    "(const ",
                    param_name,
                    "&in)",
                    is_const ? "const" : ""
                );
            }
            else if(ParamRef)
            {
                return string_concat(
                    ret_decl,
                    ' ',
                    op_name,
                    '(',
                    param_name,
                    "&)",
                    is_const ? "const" : ""
                );
            }
            else
            {
                return string_concat(
                    ret_decl,
                    ' ',
                    op_name,
                    '(',
                    param_name,
                    ')',
                    is_const ? "const" : ""
                );
            }
        }

        // User has already provided full declaration of the parameter
        constexpr static std::string gen_name_for_user_decl(
            std::string_view ret_decl,
            std::string_view op_name,
            std::string_view param_decl,
            bool is_const
        )
        {
            return string_concat(
                ret_decl,
                ' ',
                op_name,
                '(',
                param_decl,
                ')',
                is_const ? "const" : ""
            );
        }

        template <bool AutoDecl, bool ParamRef, bool ParamConst>
        static constexpr std::string gen_name(
            std::string_view ret_decl,
            std::string_view op_name,
            std::string_view param_decl_or_name,
            bool is_const
        )
        {
            if constexpr(AutoDecl)
            {
                return gen_name_for_auto_decl<ParamRef, ParamConst>(
                    ret_decl, op_name, param_decl_or_name, is_const
                );
            }
            else
            {
                return gen_name_for_user_decl(
                    ret_decl, op_name, param_decl_or_name, is_const
                );
            }
        }

        template <bool AutoDecl, typename T>
        static constexpr std::string gen_name_for(
            std::string_view ret_decl,
            std::string_view op_name,
            std::string_view param_decl_or_name,
            bool is_const
        )
        {
            if constexpr(AutoDecl)
            {
                constexpr bool param_const =
                    std::is_const_v<std::remove_reference_t<T>>;
                return gen_name_for_auto_decl<std::is_reference_v<T>, param_const>(
                    ret_decl, op_name, param_decl_or_name, is_const
                );
            }
            else
            {
                return gen_name_for_user_decl(
                    ret_decl, op_name, param_decl_or_name, is_const
                );
            }
        }
    };

#define ASBIND20_OPERATOR_RETURN_PROXY_FUNC(op_name)                        \
    const op_name* operator->() const noexcept                              \
    {                                                                       \
        return this;                                                        \
    }                                                                       \
    template <typename Return>                                              \
    [[nodiscard]]                                                           \
    return_proxy<Return> return_() const                                    \
    {                                                                       \
        return {*this};                                                     \
    }                                                                       \
    template <typename Return>                                              \
    [[nodiscard]]                                                           \
    return_proxy_with_decl<Return> return_(std::string_view ret_decl) const \
    {                                                                       \
        return {*this, ret_decl};                                           \
    }

#define ASBIND20_ASSIGNMENT_OPERATOR_HELPER(op_name, cpp_op)                                                   \
    template <bool ThisConst, typename Rhs>                                                                    \
    class op_name;                                                                                             \
                                                                                                               \
    template <bool ThisConst, bool RhsConst>                                                                   \
    class op_name<ThisConst, this_placeholder<RhsConst>> :                                                     \
        public binary_operator                                                                                 \
    {                                                                                                          \
    public:                                                                                                    \
        template <typename Return>                                                                             \
        class return_proxy                                                                                     \
        {                                                                                                      \
        public:                                                                                                \
            return_proxy(const op_name& proxy)                                                                 \
                : m_proxy(&proxy) {}                                                                           \
            template <typename RegisterHelper>                                                                 \
            void operator()(RegisterHelper& ar) const                                                          \
            {                                                                                                  \
                using class_type = typename RegisterHelper::class_type;                                        \
                using this_arg_type = std::conditional_t<                                                      \
                    ThisConst,                                                                                 \
                    std::add_const_t<class_type>,                                                              \
                    class_type>;                                                                               \
                using rhs_arg_type = std::conditional_t<                                                       \
                    RhsConst,                                                                                  \
                    std::add_const_t<class_type>,                                                              \
                    class_type>;                                                                               \
                ar.method(                                                                                     \
                    gen_name<true, true, RhsConst>(                                                            \
                        detail::get_return_decl<Return>(ar),                                                   \
                        #op_name,                                                                              \
                        ar.get_name(),                                                                         \
                        ThisConst                                                                              \
                    ),                                                                                         \
                    [](this_arg_type& lhs, rhs_arg_type& rhs) -> Return                                        \
                    {                                                                                          \
                        return lhs cpp_op rhs;                                                                 \
                    },                                                                                         \
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>                                    \
                );                                                                                             \
            }                                                                                                  \
                                                                                                               \
        private:                                                                                               \
            const op_name* m_proxy;                                                                            \
        };                                                                                                     \
        template <typename Return>                                                                             \
        class return_proxy_with_decl                                                                           \
        {                                                                                                      \
        public:                                                                                                \
            return_proxy_with_decl(const op_name& proxy, std::string_view ret_decl)                            \
                : m_proxy(&proxy), m_ret_decl(ret_decl) {}                                                     \
            template <typename RegisterHelper>                                                                 \
            void operator()(RegisterHelper& ar) const                                                          \
            {                                                                                                  \
                using class_type = typename RegisterHelper::class_type;                                        \
                using this_arg_type = std::conditional_t<                                                      \
                    ThisConst,                                                                                 \
                    std::add_const_t<class_type>,                                                              \
                    class_type>;                                                                               \
                using rhs_arg_type = std::conditional_t<                                                       \
                    RhsConst,                                                                                  \
                    std::add_const_t<class_type>,                                                              \
                    class_type>;                                                                               \
                ar.method(                                                                                     \
                    gen_name<true, true, RhsConst>(                                                            \
                        m_ret_decl,                                                                            \
                        #op_name,                                                                              \
                        ar.get_name(),                                                                         \
                        ThisConst                                                                              \
                    ),                                                                                         \
                    [](this_arg_type& lhs, rhs_arg_type& rhs) -> Return                                        \
                    {                                                                                          \
                        return lhs cpp_op rhs;                                                                 \
                    },                                                                                         \
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>                                    \
                );                                                                                             \
            }                                                                                                  \
                                                                                                               \
        private:                                                                                               \
            const op_name* m_proxy;                                                                            \
            std::string_view m_ret_decl;                                                                       \
        };                                                                                                     \
        ASBIND20_OPERATOR_RETURN_PROXY_FUNC(op_name)                                                           \
        template <typename RegisterHelper>                                                                     \
        void operator()(RegisterHelper& ar) const                                                              \
        {                                                                                                      \
            using class_type = typename RegisterHelper::class_type;                                            \
            using this_arg_type = std::conditional_t<                                                          \
                ThisConst,                                                                                     \
                std::add_const_t<class_type>,                                                                  \
                class_type>;                                                                                   \
            using rhs_arg_type = std::conditional_t<                                                           \
                RhsConst,                                                                                      \
                std::add_const_t<class_type>,                                                                  \
                class_type>;                                                                                   \
            using return_type = decltype(std::declval<this_arg_type&>() cpp_op std::declval<rhs_arg_type&>()); \
            ar.use(this->return_<return_type>());                                                              \
        }                                                                                                      \
    };                                                                                                         \
                                                                                                               \
    template <bool ThisConst, typename Rhs, bool AutoDecl>                                                     \
    class op_name<ThisConst, param_placeholder<Rhs, AutoDecl>> :                                               \
        private param_placeholder<Rhs, AutoDecl>,                                                              \
        public binary_operator                                                                                 \
    {                                                                                                          \
        using param_type = param_placeholder<Rhs, AutoDecl>;                                                   \
                                                                                                               \
    public:                                                                                                    \
        op_name(const param_type& param)                                                                       \
            : param_type(param) {}                                                                             \
        template <typename Return>                                                                             \
        class return_proxy                                                                                     \
        {                                                                                                      \
        public:                                                                                                \
            return_proxy(const op_name& proxy)                                                                 \
                : m_proxy(&proxy) {}                                                                           \
            template <typename RegisterHelper>                                                                 \
            void operator()(RegisterHelper& ar) const                                                          \
            {                                                                                                  \
                using class_type = typename RegisterHelper::class_type;                                        \
                using this_arg_type = std::conditional_t<                                                      \
                    ThisConst,                                                                                 \
                    std::add_const_t<class_type>,                                                              \
                    class_type>;                                                                               \
                ar.method(                                                                                     \
                    gen_name_for<AutoDecl, Rhs>(                                                               \
                        detail::get_return_decl<Return>(ar),                                                   \
                        #op_name,                                                                              \
                        m_proxy->param_type::get_decl(),                                                       \
                        ThisConst                                                                              \
                    ),                                                                                         \
                    [](this_arg_type& lhs, Rhs rhs) -> Return                                                  \
                    {                                                                                          \
                        return lhs cpp_op rhs;                                                                 \
                    },                                                                                         \
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>                                    \
                );                                                                                             \
            }                                                                                                  \
                                                                                                               \
        private:                                                                                               \
            const op_name* m_proxy;                                                                            \
        };                                                                                                     \
        template <typename Return>                                                                             \
        class return_proxy_with_decl                                                                           \
        {                                                                                                      \
        public:                                                                                                \
            return_proxy_with_decl(const op_name& proxy, std::string_view ret_decl)                            \
                : m_proxy(&proxy), m_ret_decl(ret_decl) {}                                                     \
            template <typename RegisterHelper>                                                                 \
            void operator()(RegisterHelper& ar) const                                                          \
            {                                                                                                  \
                using class_type = typename RegisterHelper::class_type;                                        \
                using this_arg_type = std::conditional_t<                                                      \
                    ThisConst,                                                                                 \
                    std::add_const_t<class_type>,                                                              \
                    class_type>;                                                                               \
                ar.method(                                                                                     \
                    gen_name_for<AutoDecl, Rhs>(                                                               \
                        m_ret_decl,                                                                            \
                        #op_name,                                                                              \
                        m_proxy->param_type::get_decl(),                                                       \
                        ThisConst                                                                              \
                    ),                                                                                         \
                    [](this_arg_type& lhs, Rhs rhs) -> Return                                                  \
                    {                                                                                          \
                        return lhs cpp_op rhs;                                                                 \
                    },                                                                                         \
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>                                    \
                );                                                                                             \
            }                                                                                                  \
                                                                                                               \
        private:                                                                                               \
            const op_name* m_proxy;                                                                            \
            std::string_view m_ret_decl;                                                                       \
        };                                                                                                     \
        ASBIND20_OPERATOR_RETURN_PROXY_FUNC(op_name)                                                           \
        template <typename RegisterHelper>                                                                     \
        void operator()(RegisterHelper& ar) const                                                              \
        {                                                                                                      \
            using class_type = typename RegisterHelper::class_type;                                            \
            using this_arg_type = std::conditional_t<                                                          \
                ThisConst,                                                                                     \
                std::add_const_t<class_type>,                                                                  \
                class_type>;                                                                                   \
            using return_type = decltype(std::declval<this_arg_type&>() cpp_op std::declval<Rhs>());           \
            ar.use(this->return_<return_type>());                                                              \
        }                                                                                                      \
    };

    ASBIND20_ASSIGNMENT_OPERATOR_HELPER(opAddAssign, +=)
    ASBIND20_ASSIGNMENT_OPERATOR_HELPER(opSubAssign, -=)
    ASBIND20_ASSIGNMENT_OPERATOR_HELPER(opMulAssign, *=)
    ASBIND20_ASSIGNMENT_OPERATOR_HELPER(opDivAssign, /=)
    ASBIND20_ASSIGNMENT_OPERATOR_HELPER(opAssignMod, %=)
    ASBIND20_ASSIGNMENT_OPERATOR_HELPER(opAssignAnd, &=)
    ASBIND20_ASSIGNMENT_OPERATOR_HELPER(opAssignOr, |=)
    ASBIND20_ASSIGNMENT_OPERATOR_HELPER(opAssignXor, ^=)
    ASBIND20_ASSIGNMENT_OPERATOR_HELPER(opAssignShl, <<=)
    ASBIND20_ASSIGNMENT_OPERATOR_HELPER(opAssignShr, >>=)

#undef ASBIND20_ASSIGNMENT_OPERATOR_HELPER

    class index_operator
    {
    protected:
        // The parameter needs to generate full declaration from a plain type name
        template <bool ParamRef, bool ParamConst>
        constexpr static std::string gen_name_for_auto_decl(
            std::string_view ret_decl,
            std::string_view param_name,
            bool is_const
        )
        {
            if constexpr(ParamConst && ParamRef)
            {
                return string_concat(
                    ret_decl,
                    " opIndex(const ",
                    param_name,
                    "&in)",
                    is_const ? "const" : ""
                );
            }
            else if(ParamRef)
            {
                return string_concat(
                    ret_decl,
                    " opIndex(",
                    param_name,
                    "&)",
                    is_const ? "const" : ""
                );
            }
            else
            {
                return string_concat(
                    ret_decl,
                    " opIndex(",
                    param_name,
                    ')',
                    is_const ? "const" : ""
                );
            }
        }

        // User has already provided full declaration of the parameter
        constexpr static std::string gen_name_for_user_decl(
            std::string_view ret_decl,
            std::string_view param_decl,
            bool is_const
        )
        {
            return string_concat(
                ret_decl,
                " opIndex(",
                param_decl,
                ')',
                is_const ? "const" : ""
            );
        }

        template <bool AutoDecl, bool ParamRef, bool ParamConst>
        static constexpr std::string gen_name(
            std::string_view ret_decl,
            std::string_view param_decl_or_name,
            bool is_const
        )
        {
            if constexpr(AutoDecl)
            {
                return gen_name_for_auto_decl<ParamRef, ParamConst>(
                    ret_decl, param_decl_or_name, is_const
                );
            }
            else
            {
                return gen_name_for_user_decl(
                    ret_decl, param_decl_or_name, is_const
                );
            }
        }

        template <bool AutoDecl, typename T>
        static constexpr std::string gen_name_for(
            std::string_view ret_decl,
            std::string_view param_decl_or_name,
            bool is_const
        )
        {
            if constexpr(AutoDecl)
            {
                constexpr bool param_const =
                    std::is_const_v<std::remove_reference_t<T>>;
                return gen_name_for_auto_decl<std::is_reference_v<T>, param_const>(
                    ret_decl, param_decl_or_name, is_const
                );
            }
            else
            {
                return gen_name_for_user_decl(
                    ret_decl, param_decl_or_name, is_const
                );
            }
        }
    };

    template <bool ThisConst, typename Index>
    class opIndex;

    template <bool ThisConst, bool IndexConst>
    class opIndex<ThisConst, this_placeholder<IndexConst>> : public index_operator
    {
    public:
        template <typename Return>
        class return_proxy
        {
        public:
            return_proxy(const opIndex& proxy)
                : m_proxy(&proxy) {}

            template <typename RegisterHelper>
            void operator()(RegisterHelper& ar) const
            {
                using class_type = typename RegisterHelper::class_type;
                using this_arg_type = std::conditional_t<
                    ThisConst,
                    std::add_const_t<class_type>,
                    class_type>;
                using index_arg_type = std::conditional_t<
                    IndexConst,
                    std::add_const_t<class_type>,
                    class_type>;
                ar.method(
                    gen_name_for_auto_decl<true, IndexConst>(
                        detail::get_return_decl<Return>(ar),
                        ar.get_name(),
                        ThisConst
                    ),
                    [](this_arg_type& this_, index_arg_type& idx) -> Return
                    {
                        return this_[idx];
                    },
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>
                );
            }

        private:
            const opIndex* m_proxy;
        };

        template <typename Return>
        class return_proxy_with_decl
        {
        public:
            return_proxy_with_decl(const opIndex& proxy, std::string_view ret_decl)
                : m_proxy(&proxy), m_ret_decl(ret_decl) {}

            template <typename RegisterHelper>
            void operator()(RegisterHelper& ar) const
            {
                using class_type = typename RegisterHelper::class_type;
                using this_arg_type = std::conditional_t<
                    ThisConst,
                    std::add_const_t<class_type>,
                    class_type>;
                using index_arg_type = std::conditional_t<
                    IndexConst,
                    std::add_const_t<class_type>,
                    class_type>;
                ar.method(
                    gen_name_for_auto_decl<true, IndexConst>(
                        m_ret_decl,
                        ar.get_name(),
                        ThisConst
                    ),
                    [](this_arg_type& this_, index_arg_type& idx) -> Return
                    {
                        return this_[idx];
                    },
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>
                );
            }

        private:
            const opIndex* m_proxy;
            std::string_view m_ret_decl;
        };
        ASBIND20_OPERATOR_RETURN_PROXY_FUNC(opIndex)

        template <typename RegisterHelper>
        void operator()(RegisterHelper& ar) const
        {
            using class_type = typename RegisterHelper::class_type;
            using this_arg_type = std::conditional_t<
                ThisConst,
                std::add_const_t<class_type>,
                class_type>;
            using index_arg_type = std::conditional_t<
                IndexConst,
                std::add_const_t<class_type>,
                class_type>;
            using return_type = decltype(std::declval<this_arg_type&>()[std::declval<index_arg_type&>()]);
            ar.use(this->return_<return_type>());
        }
    };

    template <bool ThisConst, typename Index, bool AutoDecl>
    class opIndex<ThisConst, param_placeholder<Index, AutoDecl>> :
        private param_placeholder<Index, AutoDecl>,
        public index_operator
    {
    public:
        using param_type = param_placeholder<Index, AutoDecl>;

    public:
        opIndex(const param_type& param)
            : param_type(param) {}

        template <typename Return>
        class return_proxy
        {
        public:
            return_proxy(const opIndex& proxy)
                : m_proxy(&proxy) {}

            template <typename RegisterHelper>
            void operator()(RegisterHelper& ar) const
            {
                using class_type = typename RegisterHelper::class_type;
                using this_arg_type = std::conditional_t<
                    ThisConst,
                    std::add_const_t<class_type>,
                    class_type>;
                ar.method(
                    gen_name_for<AutoDecl, Index>(
                        detail::get_return_decl<Return>(ar),
                        m_proxy->param_type::get_decl(),
                        ThisConst
                    ),
                    [](this_arg_type& this_, Index idx) -> Return
                    {
                        return this_[idx];
                    },
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>
                );
            }

        private:
            const opIndex* m_proxy;
        };

        template <typename Return>
        class return_proxy_with_decl
        {
        public:
            return_proxy_with_decl(const opIndex& proxy, std::string_view ret_decl)
                : m_proxy(&proxy), m_ret_decl(ret_decl) {}

            template <typename RegisterHelper>
            void operator()(RegisterHelper& ar) const
            {
                using class_type = typename RegisterHelper::class_type;
                using this_arg_type = std::conditional_t<
                    ThisConst,
                    std::add_const_t<class_type>,
                    class_type>;
                ar.method(
                    gen_name_for_user_decl(
                        m_ret_decl,
                        m_proxy->param_type::get_decl(),
                        ThisConst
                    ),
                    [](this_arg_type& this_, Index idx) -> Return
                    {
                        return this_[idx];
                    },
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>
                );
            }

        private:
            const opIndex* m_proxy;
            std::string_view m_ret_decl;
        };
        ASBIND20_OPERATOR_RETURN_PROXY_FUNC(opIndex)

        template <typename RegisterHelper>
        void operator()(RegisterHelper& ar) const
        {
            using class_type = typename RegisterHelper::class_type;
            using this_arg_type = std::conditional_t<
                ThisConst,
                std::add_const_t<class_type>,
                class_type>;
            using return_type = decltype(std::declval<this_arg_type&>()[std::declval<Index>()]);
            ar.use(this->return_<return_type>());
        }
    };
} // namespace operators

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

#define ASBIND20_ASSIGNMENT_OPERATOR_OVERLOAD(op_name, cpp_op)              \
    template <typename Rhs, bool AutoDecl>                                  \
    auto operator cpp_op(const param_placeholder<Rhs, AutoDecl>& rhs) const \
        ->operators::op_name<IsConst, param_placeholder<Rhs, AutoDecl>>     \
    {                                                                       \
        return {rhs};                                                       \
    }

    ASBIND20_ASSIGNMENT_OPERATOR_OVERLOAD(opAddAssign, +=)
    ASBIND20_ASSIGNMENT_OPERATOR_OVERLOAD(opSubAssign, -=)
    ASBIND20_ASSIGNMENT_OPERATOR_OVERLOAD(opMulAssign, *=)
    ASBIND20_ASSIGNMENT_OPERATOR_OVERLOAD(opDivAssign, /=)
    ASBIND20_ASSIGNMENT_OPERATOR_OVERLOAD(opAssignMod, %=)
    ASBIND20_ASSIGNMENT_OPERATOR_OVERLOAD(opAssignAnd, &=)
    ASBIND20_ASSIGNMENT_OPERATOR_OVERLOAD(opAssignOr, |=)
    ASBIND20_ASSIGNMENT_OPERATOR_OVERLOAD(opAssignXor, ^=)
    ASBIND20_ASSIGNMENT_OPERATOR_OVERLOAD(opAssignShl, <<=)
    ASBIND20_ASSIGNMENT_OPERATOR_OVERLOAD(opAssignShr, >>=)

#undef ASBIND20_ASSIGNMENT_OPERATOR_OVERLOAD

    template <bool IndexConst>
    auto operator[](this_placeholder<IndexConst>) const
        -> operators::opIndex<IsConst, this_placeholder<IndexConst>>
    {
        return {};
    }

    template <typename Index, bool AutoDecl>
    auto operator[](const param_placeholder<Index, AutoDecl>& idx) const
        -> operators::opIndex<IsConst, param_placeholder<Index, AutoDecl>>
    {
        return {idx};
    }
};

inline constexpr this_placeholder<false> _this;
inline constexpr this_placeholder<true> const_this;

namespace operators
{
    class unary_operator
    {
    public:
        static constexpr std::size_t operand_count = 1;

    protected:
        constexpr static std::string gen_name(
            std::string_view ret_decl,
            std::string_view op_name,
            bool is_const
        )
        {
            return string_concat(
                ret_decl,
                ' ',
                op_name,
                is_const ? "()const" : "()"
            );
        }
    };

#define ASBIND20_PREFIX_UNARY_OPERATOR(op_name, cpp_op)                             \
    template <bool ThisConst>                                                       \
    class op_name : public unary_operator                                           \
    {                                                                               \
    public:                                                                         \
        template <typename Return>                                                  \
        class return_proxy                                                          \
        {                                                                           \
        public:                                                                     \
            return_proxy(const op_name& proxy)                                      \
                : m_proxy(&proxy) {}                                                \
            template <typename RegisterHelper>                                      \
            void operator()(RegisterHelper& ar) const                               \
            {                                                                       \
                using class_type = typename RegisterHelper::class_type;             \
                using this_arg_type = std::conditional_t<                           \
                    ThisConst,                                                      \
                    std::add_const_t<class_type>,                                   \
                    class_type>;                                                    \
                ar.method(                                                          \
                    gen_name(                                                       \
                        detail::get_return_decl<Return>(ar),                        \
                        #op_name,                                                   \
                        ThisConst                                                   \
                    ),                                                              \
                    [](this_arg_type& this_) -> Return                              \
                    {                                                               \
                        return cpp_op std::move(this_);                             \
                    },                                                              \
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>          \
                );                                                                  \
            }                                                                       \
                                                                                    \
        private:                                                                    \
            const op_name* m_proxy;                                                 \
        };                                                                          \
        template <typename Return>                                                  \
        class return_proxy_with_decl                                                \
        {                                                                           \
        public:                                                                     \
            return_proxy_with_decl(const op_name& proxy, std::string_view ret_decl) \
                : m_proxy(&proxy), m_ret_decl(ret_decl) {}                          \
            template <typename RegisterHelper>                                      \
            void operator()(RegisterHelper& ar) const                               \
            {                                                                       \
                using class_type = typename RegisterHelper::class_type;             \
                using this_arg_type = std::conditional_t<                           \
                    ThisConst,                                                      \
                    std::add_const_t<class_type>,                                   \
                    class_type>;                                                    \
                ar.method(                                                          \
                    gen_name(                                                       \
                        m_ret_decl,                                                 \
                        #op_name,                                                   \
                        ThisConst                                                   \
                    ),                                                              \
                    [](this_arg_type& this_) -> Return                              \
                    {                                                               \
                        return cpp_op std::move(this_);                             \
                    },                                                              \
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>          \
                );                                                                  \
            }                                                                       \
                                                                                    \
        private:                                                                    \
            const op_name* m_proxy;                                                 \
            std::string_view m_ret_decl;                                            \
        };                                                                          \
        ASBIND20_OPERATOR_RETURN_PROXY_FUNC(op_name)                                \
        template <typename RegisterHelper>                                          \
        void operator()(RegisterHelper& ar) const                                   \
        {                                                                           \
            using class_type = typename RegisterHelper::class_type;                 \
            using this_arg_type = std::conditional_t<                               \
                ThisConst,                                                          \
                std::add_const_t<class_type>,                                       \
                class_type>;                                                        \
            using return_type = decltype(cpp_op std::declval<this_arg_type&>());    \
            ar.use(this->return_<return_type>());                                   \
        }                                                                           \
    };

#define ASBIND20_SUFFIX_UNARY_OPERATOR(op_name, cpp_op)                             \
    template <bool ThisConst>                                                       \
    class op_name : public unary_operator                                           \
    {                                                                               \
    public:                                                                         \
        template <typename Return>                                                  \
        class return_proxy                                                          \
        {                                                                           \
        public:                                                                     \
            return_proxy(const op_name& proxy)                                      \
                : m_proxy(&proxy) {}                                                \
            template <typename RegisterHelper>                                      \
            void operator()(RegisterHelper& ar) const                               \
            {                                                                       \
                using class_type = typename RegisterHelper::class_type;             \
                using this_arg_type = std::conditional_t<                           \
                    ThisConst,                                                      \
                    std::add_const_t<class_type>,                                   \
                    class_type>;                                                    \
                ar.method(                                                          \
                    gen_name(                                                       \
                        detail::get_return_decl<Return>(ar),                        \
                        #op_name,                                                   \
                        ThisConst                                                   \
                    ),                                                              \
                    [](this_arg_type& this_) -> Return                              \
                    {                                                               \
                        return std::move(this_) cpp_op;                             \
                    },                                                              \
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>          \
                );                                                                  \
            }                                                                       \
                                                                                    \
        private:                                                                    \
            const op_name* m_proxy;                                                 \
        };                                                                          \
        template <typename Return>                                                  \
        class return_proxy_with_decl                                                \
        {                                                                           \
        public:                                                                     \
            return_proxy_with_decl(const op_name& proxy, std::string_view ret_decl) \
                : m_proxy(&proxy), m_ret_decl(ret_decl) {}                          \
            template <typename RegisterHelper>                                      \
            void operator()(RegisterHelper& ar) const                               \
            {                                                                       \
                using class_type = typename RegisterHelper::class_type;             \
                using this_arg_type = std::conditional_t<                           \
                    ThisConst,                                                      \
                    std::add_const_t<class_type>,                                   \
                    class_type>;                                                    \
                                                                                    \
                ar.method(                                                          \
                    gen_name(                                                       \
                        m_ret_decl,                                                 \
                        #op_name,                                                   \
                        ThisConst                                                   \
                    ),                                                              \
                    [](this_arg_type& this_) -> Return                              \
                    {                                                               \
                        return std::move(this_) cpp_op;                             \
                    },                                                              \
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>          \
                );                                                                  \
            }                                                                       \
                                                                                    \
        private:                                                                    \
            const op_name* m_proxy;                                                 \
            std::string_view m_ret_decl;                                            \
        };                                                                          \
        ASBIND20_OPERATOR_RETURN_PROXY_FUNC(op_name)                                \
        template <typename RegisterHelper>                                          \
        void operator()(RegisterHelper& ar) const                                   \
        {                                                                           \
            using class_type = typename RegisterHelper::class_type;                 \
            using this_arg_type = std::conditional_t<                               \
                ThisConst,                                                          \
                std::add_const_t<class_type>,                                       \
                class_type>;                                                        \
            using return_type = decltype(std::declval<this_arg_type&>() cpp_op);    \
            ar.use(this->return_<return_type>());                                   \
        }                                                                           \
    };

    ASBIND20_PREFIX_UNARY_OPERATOR(opNeg, -);
    ASBIND20_PREFIX_UNARY_OPERATOR(opCom, ~);
    ASBIND20_PREFIX_UNARY_OPERATOR(opPreInc, ++);
    ASBIND20_PREFIX_UNARY_OPERATOR(opPreDec, --);
    ASBIND20_SUFFIX_UNARY_OPERATOR(opPostInc, ++);
    ASBIND20_SUFFIX_UNARY_OPERATOR(opPostDec, --);

#undef ASBIND20_PREFIX_UNARY_OPERATOR


#define ASBIND20_BINARY_OPERATOR_HELPER(op_name, cpp_op)                                                       \
    template <typename Lhs, typename Rhs>                                                                      \
    class op_name;                                                                                             \
                                                                                                               \
    template <bool LhsConst, bool RhsConst>                                                                    \
    class op_name<this_placeholder<LhsConst>, this_placeholder<RhsConst>> :                                    \
        public binary_operator                                                                                 \
    {                                                                                                          \
    public:                                                                                                    \
        template <typename Return>                                                                             \
        class return_proxy                                                                                     \
        {                                                                                                      \
        public:                                                                                                \
            return_proxy(const op_name& proxy)                                                                 \
                : m_proxy(&proxy) {}                                                                           \
            template <typename RegisterHelper>                                                                 \
            void operator()(RegisterHelper& ar) const                                                          \
            {                                                                                                  \
                using class_type = typename RegisterHelper::class_type;                                        \
                using this_arg_type = std::conditional_t<                                                      \
                    LhsConst,                                                                                  \
                    std::add_const_t<class_type>,                                                              \
                    class_type>;                                                                               \
                using rhs_arg_type = std::conditional_t<                                                       \
                    RhsConst,                                                                                  \
                    std::add_const_t<class_type>,                                                              \
                    class_type>;                                                                               \
                ar.method(                                                                                     \
                    gen_name<true, true, RhsConst>(                                                            \
                        detail::get_return_decl<Return>(ar),                                                   \
                        #op_name,                                                                              \
                        ar.get_name(),                                                                         \
                        LhsConst                                                                               \
                    ),                                                                                         \
                    [](this_arg_type& lhs, rhs_arg_type& rhs) -> Return                                        \
                    {                                                                                          \
                        return lhs cpp_op rhs;                                                                 \
                    },                                                                                         \
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>                                    \
                );                                                                                             \
            }                                                                                                  \
                                                                                                               \
        private:                                                                                               \
            const op_name* m_proxy;                                                                            \
        };                                                                                                     \
        template <typename Return>                                                                             \
        class return_proxy_with_decl                                                                           \
        {                                                                                                      \
        public:                                                                                                \
            return_proxy_with_decl(const op_name& proxy, std::string_view ret_decl)                            \
                : m_proxy(&proxy), m_ret_decl(ret_decl) {}                                                     \
            template <typename RegisterHelper>                                                                 \
            void operator()(RegisterHelper& ar) const                                                          \
            {                                                                                                  \
                using class_type = typename RegisterHelper::class_type;                                        \
                using this_arg_type = std::conditional_t<                                                      \
                    LhsConst,                                                                                  \
                    std::add_const_t<class_type>,                                                              \
                    class_type>;                                                                               \
                using rhs_arg_type = std::conditional_t<                                                       \
                    RhsConst,                                                                                  \
                    std::add_const_t<class_type>,                                                              \
                    class_type>;                                                                               \
                ar.method(                                                                                     \
                    gen_name<true, true, RhsConst>(                                                            \
                        m_ret_decl,                                                                            \
                        #op_name,                                                                              \
                        ar.get_name(),                                                                         \
                        LhsConst                                                                               \
                    ),                                                                                         \
                    [](this_arg_type& lhs, rhs_arg_type& rhs) -> Return                                        \
                    {                                                                                          \
                        return lhs cpp_op rhs;                                                                 \
                    },                                                                                         \
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>                                    \
                );                                                                                             \
            }                                                                                                  \
                                                                                                               \
        private:                                                                                               \
            const op_name* m_proxy;                                                                            \
            std::string_view m_ret_decl;                                                                       \
        };                                                                                                     \
        ASBIND20_OPERATOR_RETURN_PROXY_FUNC(op_name)                                                           \
        template <typename RegisterHelper>                                                                     \
        void operator()(RegisterHelper& ar) const                                                              \
        {                                                                                                      \
            using class_type = typename RegisterHelper::class_type;                                            \
            using this_arg_type = std::conditional_t<                                                          \
                LhsConst,                                                                                      \
                std::add_const_t<class_type>,                                                                  \
                class_type>;                                                                                   \
            using rhs_arg_type = std::conditional_t<                                                           \
                RhsConst,                                                                                      \
                std::add_const_t<class_type>,                                                                  \
                class_type>;                                                                                   \
            using return_type = decltype(std::declval<this_arg_type&>() cpp_op std::declval<rhs_arg_type&>()); \
            ar.use(this->return_<return_type>());                                                              \
        }                                                                                                      \
    };                                                                                                         \
                                                                                                               \
    template <bool LhsConst, typename Rhs, bool AutoDecl>                                                      \
    class op_name<this_placeholder<LhsConst>, param_placeholder<Rhs, AutoDecl>> :                              \
        private param_placeholder<Rhs, AutoDecl>,                                                              \
        public binary_operator                                                                                 \
    {                                                                                                          \
        using param_type = param_placeholder<Rhs, AutoDecl>;                                                   \
                                                                                                               \
    public:                                                                                                    \
        op_name(const param_type& param)                                                                       \
            : param_type(param) {}                                                                             \
        template <typename Return>                                                                             \
        class return_proxy                                                                                     \
        {                                                                                                      \
        public:                                                                                                \
            return_proxy(const op_name& proxy)                                                                 \
                : m_proxy(&proxy) {}                                                                           \
            template <typename RegisterHelper>                                                                 \
            void operator()(RegisterHelper& ar) const                                                          \
            {                                                                                                  \
                using class_type = typename RegisterHelper::class_type;                                        \
                using this_arg_type = std::conditional_t<                                                      \
                    LhsConst,                                                                                  \
                    std::add_const_t<class_type>,                                                              \
                    class_type>;                                                                               \
                ar.method(                                                                                     \
                    gen_name_for<AutoDecl, Rhs>(                                                               \
                        detail::get_return_decl<Return>(ar),                                                   \
                        #op_name,                                                                              \
                        m_proxy->param_type::get_decl(),                                                       \
                        LhsConst                                                                               \
                    ),                                                                                         \
                    [](this_arg_type& lhs, Rhs rhs) -> Return                                                  \
                    {                                                                                          \
                        return lhs cpp_op rhs;                                                                 \
                    },                                                                                         \
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>                                    \
                );                                                                                             \
            }                                                                                                  \
                                                                                                               \
        private:                                                                                               \
            const op_name* m_proxy;                                                                            \
        };                                                                                                     \
        template <typename Return>                                                                             \
        class return_proxy_with_decl                                                                           \
        {                                                                                                      \
        public:                                                                                                \
            return_proxy_with_decl(const op_name& proxy, std::string_view ret_decl)                            \
                : m_proxy(&proxy), m_ret_decl(ret_decl) {}                                                     \
            template <typename RegisterHelper>                                                                 \
            void operator()(RegisterHelper& ar) const                                                          \
            {                                                                                                  \
                using class_type = typename RegisterHelper::class_type;                                        \
                using this_arg_type = std::conditional_t<                                                      \
                    LhsConst,                                                                                  \
                    std::add_const_t<class_type>,                                                              \
                    class_type>;                                                                               \
                ar.method(                                                                                     \
                    gen_name_for<AutoDecl, Rhs>(                                                               \
                        m_ret_decl,                                                                            \
                        #op_name,                                                                              \
                        m_proxy->param_type::get_decl(),                                                       \
                        LhsConst                                                                               \
                    ),                                                                                         \
                    [](this_arg_type& lhs, Rhs rhs) -> Return                                                  \
                    {                                                                                          \
                        return lhs cpp_op rhs;                                                                 \
                    },                                                                                         \
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST>                                    \
                );                                                                                             \
            }                                                                                                  \
                                                                                                               \
        private:                                                                                               \
            const op_name* m_proxy;                                                                            \
            std::string_view m_ret_decl;                                                                       \
        };                                                                                                     \
        ASBIND20_OPERATOR_RETURN_PROXY_FUNC(op_name)                                                           \
        template <typename RegisterHelper>                                                                     \
        void operator()(RegisterHelper& ar) const                                                              \
        {                                                                                                      \
            using class_type = typename RegisterHelper::class_type;                                            \
            using this_arg_type = std::conditional_t<                                                          \
                LhsConst,                                                                                      \
                std::add_const_t<class_type>,                                                                  \
                class_type>;                                                                                   \
            using return_type = decltype(std::declval<this_arg_type&>() cpp_op std::declval<Rhs>());           \
            ar.use(this->return_<return_type>());                                                              \
        }                                                                                                      \
    };                                                                                                         \
                                                                                                               \
    template <typename Lhs, bool AutoDecl, bool RhsConst>                                                      \
    class op_name<param_placeholder<Lhs, AutoDecl>, this_placeholder<RhsConst>> :                              \
        private param_placeholder<Lhs, AutoDecl>,                                                              \
        public binary_operator                                                                                 \
    {                                                                                                          \
        using param_type = param_placeholder<Lhs, AutoDecl>;                                                   \
                                                                                                               \
    public:                                                                                                    \
        op_name(const param_type& param)                                                                       \
            : param_type(param) {}                                                                             \
        template <typename Return>                                                                             \
        class return_proxy                                                                                     \
        {                                                                                                      \
        public:                                                                                                \
            return_proxy(const op_name& proxy)                                                                 \
                : m_proxy(&proxy) {}                                                                           \
            template <typename RegisterHelper>                                                                 \
            void operator()(RegisterHelper& ar) const                                                          \
            {                                                                                                  \
                using class_type = typename RegisterHelper::class_type;                                        \
                using this_arg_type = std::conditional_t<                                                      \
                    RhsConst,                                                                                  \
                    std::add_const_t<class_type>,                                                              \
                    class_type>;                                                                               \
                ar.method(                                                                                     \
                    gen_name_for<AutoDecl, Lhs>(                                                               \
                        detail::get_return_decl<Return>(ar),                                                   \
                        #op_name "_r",                                                                         \
                        m_proxy->param_type::get_decl(),                                                       \
                        RhsConst                                                                               \
                    ),                                                                                         \
                    [](Lhs lhs, this_arg_type& rhs) -> Return                                                  \
                    {                                                                                          \
                        return lhs cpp_op rhs;                                                                 \
                    },                                                                                         \
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>                                     \
                );                                                                                             \
            }                                                                                                  \
                                                                                                               \
        private:                                                                                               \
            const op_name* m_proxy;                                                                            \
        };                                                                                                     \
        template <typename Return>                                                                             \
        class return_proxy_with_decl                                                                           \
        {                                                                                                      \
        public:                                                                                                \
            return_proxy_with_decl(const op_name& proxy, std::string_view ret_decl)                            \
                : m_proxy(&proxy), m_ret_decl(ret_decl) {}                                                     \
            template <typename RegisterHelper>                                                                 \
            void operator()(RegisterHelper& ar) const                                                          \
            {                                                                                                  \
                using class_type = typename RegisterHelper::class_type;                                        \
                using this_arg_type = std::conditional_t<                                                      \
                    RhsConst,                                                                                  \
                    std::add_const_t<class_type>,                                                              \
                    class_type>;                                                                               \
                ar.method(                                                                                     \
                    gen_name_for<AutoDecl, Lhs>(                                                               \
                        m_ret_decl,                                                                            \
                        #op_name "_r",                                                                         \
                        m_proxy->param_type::get_decl(),                                                       \
                        RhsConst                                                                               \
                    ),                                                                                         \
                    [](Lhs lhs, this_arg_type& rhs) -> Return                                                  \
                    {                                                                                          \
                        return lhs cpp_op rhs;                                                                 \
                    },                                                                                         \
                    call_conv<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST>                                     \
                );                                                                                             \
            }                                                                                                  \
                                                                                                               \
        private:                                                                                               \
            const op_name* m_proxy;                                                                            \
            std::string_view m_ret_decl;                                                                       \
        };                                                                                                     \
        ASBIND20_OPERATOR_RETURN_PROXY_FUNC(op_name)                                                           \
        template <typename RegisterHelper>                                                                     \
        void operator()(RegisterHelper& ar) const                                                              \
        {                                                                                                      \
            using class_type = typename RegisterHelper::class_type;                                            \
            using this_arg_type = std::conditional_t<                                                          \
                RhsConst,                                                                                      \
                std::add_const_t<class_type>,                                                                  \
                class_type>;                                                                                   \
            using return_type = decltype(std::declval<Lhs>() cpp_op std::declval<this_arg_type&>());           \
            ar.use(this->return_<return_type>());                                                              \
        }                                                                                                      \
    };

    // Skip opPow (**) and opUShr/l (>>>/<<<) because their C++ counterparts are not existed
    ASBIND20_BINARY_OPERATOR_HELPER(opAdd, +)
    ASBIND20_BINARY_OPERATOR_HELPER(opSub, -)
    ASBIND20_BINARY_OPERATOR_HELPER(opMul, *)
    ASBIND20_BINARY_OPERATOR_HELPER(opDiv, /)
    ASBIND20_BINARY_OPERATOR_HELPER(opMod, %)
    ASBIND20_BINARY_OPERATOR_HELPER(opAnd, &)
    ASBIND20_BINARY_OPERATOR_HELPER(opOr, |)
    ASBIND20_BINARY_OPERATOR_HELPER(opXor, ^)
    ASBIND20_BINARY_OPERATOR_HELPER(opShl, <<)
    ASBIND20_BINARY_OPERATOR_HELPER(opShr, >>)

#undef ASBIND20_OPERATOR_RETURN_PROXY_FUNC
#undef ASBIND20_BINARY_OPERATOR_HELPER
} // namespace operators

#define ASBIND20_PREFIX_UNARY_OPERATOR_OVERLOAD(op_name, cpp_op) \
    template <bool ThisConst>                                    \
    constexpr auto operator cpp_op(this_placeholder<ThisConst>)  \
        ->operators::op_name<ThisConst>                          \
    {                                                            \
        return {};                                               \
    }

#define ASBIND20_SUFFIX_UNARY_OPERATOR_OVERLOAD(op_name, cpp_op)     \
    template <bool ThisConst>                                        \
    constexpr auto operator cpp_op(this_placeholder<ThisConst>, int) \
        ->operators::op_name<ThisConst>                              \
    {                                                                \
        return {};                                                   \
    }

ASBIND20_PREFIX_UNARY_OPERATOR_OVERLOAD(opNeg, -);
ASBIND20_PREFIX_UNARY_OPERATOR_OVERLOAD(opCom, ~);
ASBIND20_PREFIX_UNARY_OPERATOR_OVERLOAD(opPreInc, ++);
ASBIND20_PREFIX_UNARY_OPERATOR_OVERLOAD(opPreDec, --);
ASBIND20_SUFFIX_UNARY_OPERATOR_OVERLOAD(opPostInc, ++);
ASBIND20_SUFFIX_UNARY_OPERATOR_OVERLOAD(opPostDec, --);

#undef ASBIND20_PREFIX_UNARY_OPERATOR_OVERLOAD
#undef ASBIND20_SUFFIX_UNARY_OPERATOR_OVERLOAD

#define ASBIND20_BINARY_OPERATOR_OVERLOAD(op_name, cpp_op)                                                  \
    template <bool LhsConst, bool RhsConst>                                                                 \
    constexpr auto operator cpp_op(this_placeholder<LhsConst>, this_placeholder<RhsConst>)                  \
        ->operators::op_name<this_placeholder<LhsConst>, this_placeholder<RhsConst>>                        \
    {                                                                                                       \
        return {};                                                                                          \
    }                                                                                                       \
    template <bool LhsConst, typename Rhs, bool AutoDecl>                                                   \
    constexpr auto operator cpp_op(this_placeholder<LhsConst>, const param_placeholder<Rhs, AutoDecl>& rhs) \
        ->operators::op_name<this_placeholder<LhsConst>, param_placeholder<Rhs, AutoDecl>>                  \
    {                                                                                                       \
        return {rhs};                                                                                       \
    }                                                                                                       \
    template <typename Lhs, bool AutoDecl, bool RhsConst>                                                   \
    constexpr auto operator cpp_op(const param_placeholder<Lhs, AutoDecl>& lhs, this_placeholder<RhsConst>) \
        ->operators::op_name<param_placeholder<Lhs, AutoDecl>, this_placeholder<RhsConst>>                  \
    {                                                                                                       \
        return {lhs};                                                                                       \
    }

ASBIND20_BINARY_OPERATOR_OVERLOAD(opAdd, +)
ASBIND20_BINARY_OPERATOR_OVERLOAD(opSub, -)
ASBIND20_BINARY_OPERATOR_OVERLOAD(opMul, *)
ASBIND20_BINARY_OPERATOR_OVERLOAD(opDiv, /)
ASBIND20_BINARY_OPERATOR_OVERLOAD(opMod, %)
ASBIND20_BINARY_OPERATOR_OVERLOAD(opAnd, &)
ASBIND20_BINARY_OPERATOR_OVERLOAD(opOr, |)
ASBIND20_BINARY_OPERATOR_OVERLOAD(opXor, ^)
ASBIND20_BINARY_OPERATOR_OVERLOAD(opShl, <<)
ASBIND20_BINARY_OPERATOR_OVERLOAD(opShr, >>)

#undef ASBIND20_BINARY_OPERATOR_OVERLOAD
} // namespace asbind20

#endif
