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

#include "bind/common.hpp"
#include "meta.hpp"
#include "utility.hpp"

namespace asbind20
{

template <bool IsConst>
struct this_placeholder;

template <typename T, bool AutoDecl = false>
struct param_placeholder;

template <typename T>
struct param_placeholder<T, false>
{
    param_placeholder() = delete;
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
    template <typename T>
    constexpr std::string format_full_typename(std::string_view type_decl)
    {
        constexpr bool is_const = std::is_const_v<std::remove_reference_t<T>>;
        constexpr bool is_ref = std::is_reference_v<T>;

        if constexpr(is_const && is_ref)
            return string_concat("const ", type_decl, '&');
        else if constexpr(is_ref)
            return string_concat(type_decl, '&');
        else
            return std::string(type_decl);
    }

    template <typename T, typename BindingGenerator>
    constexpr std::string get_return_decl(const BindingGenerator& gen)
    {
        constexpr bool is_this_type = std::same_as<
            std::remove_cvref_t<T>,
            typename BindingGenerator::class_type>;

        if constexpr(!is_this_type)
        {
            auto name = name_of<std::remove_cvref_t<T>>();
            return format_full_typename<T>(name.view());
        }
        else
            return format_full_typename<T>(gen.get_name());
    }

    // Declaration generation helpers (consolidated from old base classes)

    // Binary operators: generate full declaration from plain type name
    template <bool ParamRef, bool ParamConst>
    constexpr std::string gen_binary_auto_decl(
        std::string_view ret_decl,
        std::string_view op_name,
        std::string_view param_name,
        bool is_const
    )
    {
        if constexpr(ParamConst && ParamRef)
            return string_concat(ret_decl, ' ', op_name, "(const ", param_name, "&in)", is_const ? "const" : "");
        else if constexpr(ParamRef)
            return string_concat(ret_decl, ' ', op_name, '(', param_name, "&)", is_const ? "const" : "");
        else
            return string_concat(ret_decl, ' ', op_name, '(', param_name, ')', is_const ? "const" : "");
    }

    // Binary operators: generate full declaration from user-provided param decl
    constexpr std::string gen_binary_user_decl(
        std::string_view ret_decl,
        std::string_view op_name,
        std::string_view param_decl,
        bool is_const
    )
    {
        return string_concat(ret_decl, ' ', op_name, '(', param_decl, ')', is_const ? "const" : "");
    }

    // Index operators: generate full declaration from plain type name
    template <bool ParamRef, bool ParamConst>
    constexpr std::string gen_index_auto_decl(
        std::string_view ret_decl,
        std::string_view param_name,
        bool is_const
    )
    {
        if constexpr(ParamConst && ParamRef)
            return string_concat(ret_decl, " opIndex(const ", param_name, "&in)", is_const ? "const" : "");
        else if constexpr(ParamRef)
            return string_concat(ret_decl, " opIndex(", param_name, "&)", is_const ? "const" : "");
        else
            return string_concat(ret_decl, " opIndex(", param_name, ')', is_const ? "const" : "");
    }

    // Index operators: generate full declaration from user-provided param decl
    constexpr std::string gen_index_user_decl(
        std::string_view ret_decl,
        std::string_view param_decl,
        bool is_const
    )
    {
        return string_concat(ret_decl, " opIndex(", param_decl, ')', is_const ? "const" : "");
    }

    // Unary operators: generate full declaration
    constexpr std::string gen_unary_decl(
        std::string_view ret_decl,
        std::string_view op_name,
        bool is_const
    )
    {
        return string_concat(ret_decl, ' ', op_name, is_const ? "()const" : "()");
    }

    // Dispatch between auto-decl and user-decl for binary operators
    template <bool AutoDecl, typename T>
    constexpr std::string gen_binary_param_decl(
        std::string_view ret_decl,
        std::string_view op_name,
        std::string_view param_decl_or_name,
        bool is_const
    )
    {
        if constexpr(AutoDecl)
        {
            constexpr bool param_ref = std::is_reference_v<T>;
            constexpr bool param_const = std::is_const_v<std::remove_reference_t<T>>;
            return gen_binary_auto_decl<param_ref, param_const>(
                ret_decl, op_name, param_decl_or_name, is_const
            );
        }
        else
        {
            return gen_binary_user_decl(ret_decl, op_name, param_decl_or_name, is_const);
        }
    }

    // Dispatch between auto-decl and user-decl for index operators
    template <bool AutoDecl, typename T>
    constexpr std::string gen_index_param_decl(
        std::string_view ret_decl,
        std::string_view param_decl_or_name,
        bool is_const
    )
    {
        if constexpr(AutoDecl)
        {
            constexpr bool param_ref = std::is_reference_v<T>;
            constexpr bool param_const = std::is_const_v<std::remove_reference_t<T>>;
            return gen_index_auto_decl<param_ref, param_const>(
                ret_decl, param_decl_or_name, is_const
            );
        }
        else
        {
            return gen_index_user_decl(ret_decl, param_decl_or_name, is_const);
        }
    }

    template <typename ClassType, bool IsConst>
    using this_arg_t = std::conditional_t<IsConst, std::add_const_t<ClassType>, ClassType>;

    // Operator tags — extract lambda creation logic

    // Binary/assignment operator tags
#define ASBIND20_OP_TAG_BINARY(Name, Op)                \
    struct Name                                         \
    {                                                   \
        static constexpr std::string_view name = #Name; \
        template <typename Lhs, typename Rhs>           \
        static decltype(auto) apply(Lhs& lhs, Rhs& rhs) \
        {                                               \
            return lhs Op rhs;                          \
        }                                               \
    }

    ASBIND20_OP_TAG_BINARY(opAddAssign, +=);
    ASBIND20_OP_TAG_BINARY(opSubAssign, -=);
    ASBIND20_OP_TAG_BINARY(opMulAssign, *=);
    ASBIND20_OP_TAG_BINARY(opDivAssign, /=);
    ASBIND20_OP_TAG_BINARY(opModAssign, %=);
    ASBIND20_OP_TAG_BINARY(opAndAssign, &=);
    ASBIND20_OP_TAG_BINARY(opOrAssign, |=);
    ASBIND20_OP_TAG_BINARY(opXorAssign, ^=);
    ASBIND20_OP_TAG_BINARY(opShlAssign, <<=);
    ASBIND20_OP_TAG_BINARY(opShrAssign, >>=);

    ASBIND20_OP_TAG_BINARY(opAdd, +);
    ASBIND20_OP_TAG_BINARY(opSub, -);
    ASBIND20_OP_TAG_BINARY(opMul, *);
    ASBIND20_OP_TAG_BINARY(opDiv, /);
    ASBIND20_OP_TAG_BINARY(opMod, %);
    ASBIND20_OP_TAG_BINARY(opAnd, &);
    ASBIND20_OP_TAG_BINARY(opOr, |);
    ASBIND20_OP_TAG_BINARY(opXor, ^);
    ASBIND20_OP_TAG_BINARY(opShl, <<);
    ASBIND20_OP_TAG_BINARY(opShr, >>);

#undef ASBIND20_OP_TAG_BINARY

    // opCmp — always returns int, uses three-way comparison
    struct op_cmp
    {
        static constexpr std::string_view name = "opCmp";

        template <typename L, typename R>
        static int apply(L& l, R& r)
        {
            return translate_three_way(l <=> r);
        }
    };

    // Unary operator tags
#define ASBIND20_OP_TAG_UNARY_PREFIX(Name, Op)          \
    struct Name                                         \
    {                                                   \
        static constexpr std::string_view name = #Name; \
        template <typename T>                           \
        static decltype(auto) apply(T& v)               \
        {                                               \
            return Op v;                                \
        }                                               \
    }

#define ASBIND20_OP_TAG_UNARY_SUFFIX(Name, Op)          \
    struct Name                                         \
    {                                                   \
        static constexpr std::string_view name = #Name; \
        template <typename T>                           \
        static decltype(auto) apply(T& v)               \
        {                                               \
            return v Op;                                \
        }                                               \
    }

    ASBIND20_OP_TAG_UNARY_PREFIX(opNeg, -);
    ASBIND20_OP_TAG_UNARY_PREFIX(opCom, ~);
    ASBIND20_OP_TAG_UNARY_PREFIX(opPreInc, ++);
    ASBIND20_OP_TAG_UNARY_PREFIX(opPreDec, --);
    ASBIND20_OP_TAG_UNARY_SUFFIX(opPostInc, ++);
    ASBIND20_OP_TAG_UNARY_SUFFIX(opPostDec, --);

#undef ASBIND20_OP_TAG_UNARY_PREFIX
#undef ASBIND20_OP_TAG_UNARY_SUFFIX

    // CRTP infrastructure: return_proxy + operator_base

    template <typename Parent, typename Return, bool WithDecl>
    class return_proxy;

    // return_proxy without user-provided return declaration
    template <typename Parent, typename Return>
    class return_proxy<Parent, Return, false>
    {
    public:
        explicit return_proxy(const Parent& p) : m_parent(&p) {}

        template <typename RegisterHelper>
        void operator()(RegisterHelper& ar) const
        {
            m_parent->template register_impl<Return>(
                ar, detail::get_return_decl<Return>(ar)
            );
        }

    private:
        const Parent* m_parent;
    };

    // return_proxy with user-provided return declaration
    template <typename Parent, typename Return>
    class return_proxy<Parent, Return, true>
    {
    public:
        return_proxy(const Parent& p, std::string_view ret_decl)
            : m_parent(&p), m_ret_decl(ret_decl) {}

        template <typename RegisterHelper>
        void operator()(RegisterHelper& ar) const
        {
            m_parent->template register_impl<Return>(ar, m_ret_decl);
        }

    private:
        const Parent* m_parent;
        std::string_view m_ret_decl;
    };

    // CRTP base for all operator classes
    // HasReturnDecl=true: provides both return_<T>() and return_<T>(decl)
    // HasReturnDecl=false: provides only return_<T>() (used by opCmp)
    template <typename Derived, bool HasReturnDecl = true>
    class operator_base
    {
    public:
        [[nodiscard]]
        const Derived* operator->() const noexcept
        {
            return static_cast<const Derived*>(this);
        }

        template <typename Return>
        [[nodiscard]]
        auto return_() const
        {
            return return_proxy<Derived, Return, false>{
                static_cast<const Derived&>(*this)
            };
        }

        template <typename Return>
        [[nodiscard]]
        auto return_(std::string_view ret_decl) const
            requires(HasReturnDecl)
        {
            return return_proxy<Derived, Return, true>{
                static_cast<const Derived&>(*this), ret_decl
            };
        }

        template <typename RegisterHelper>
        void operator()(RegisterHelper& ar) const
        {
            using return_type = typename Derived::template deduce_return_t<RegisterHelper>;
            ar.use(static_cast<const Derived*>(this)->template return_<return_type>());
        }
    };

    // Binary operator class templates (serve ALL assignment + binary operators)

    // Case 1: this OP this (both operands are the class type)
    template <typename OpTag, bool LhsConst, bool RhsConst>
    class binary_this_this_op : public operator_base<binary_this_this_op<OpTag, LhsConst, RhsConst>, true>
    {
    public:
        template <typename Return, typename RegisterHelper>
        void register_impl(RegisterHelper& ar, std::string_view ret_decl) const
        {
            using class_type = typename RegisterHelper::class_type;
            using L = this_arg_t<class_type, LhsConst>;
            using R = this_arg_t<class_type, RhsConst>;

            auto decl = detail::gen_binary_auto_decl</*Ref=*/true, RhsConst>(
                ret_decl, OpTag::name, ar.get_name(), LhsConst
            );

            ar.method(
                decl,
                [](L& l, R& r) -> Return
                { return OpTag::apply(l, r); },
                objfirst
            );
        }

        template <typename RegisterHelper>
        using deduce_return_t = decltype(OpTag::apply(
            std::declval<this_arg_t<typename RegisterHelper::class_type, LhsConst>&>(),
            std::declval<this_arg_t<typename RegisterHelper::class_type, RhsConst>&>()
        ));
    };

    // Case 2: this OP param (LHS is class type, RHS is arbitrary type)
    template <typename OpTag, bool ThisConst, typename Rhs, bool AutoDecl>
    class binary_this_param_op :
        private param_placeholder<Rhs, AutoDecl>,
        public operator_base<binary_this_param_op<OpTag, ThisConst, Rhs, AutoDecl>, true>
    {
        using param_type = param_placeholder<Rhs, AutoDecl>;

    public:
        binary_this_param_op(const param_type& p) : param_type(p) {}

        using operator_base<binary_this_param_op, true>::operator();

        template <typename Return, typename RegisterHelper>
        void register_impl(RegisterHelper& ar, std::string_view ret_decl) const
        {
            using class_type = typename RegisterHelper::class_type;
            using L = this_arg_t<class_type, ThisConst>;

            auto decl = detail::gen_binary_param_decl<AutoDecl, Rhs>(
                ret_decl, OpTag::name, this->param_type::get_decl(), ThisConst
            );

            ar.method(
                decl,
                [](L& l, Rhs r) -> Return
                { return OpTag::apply(l, r); },
                objfirst
            );
        }

        template <typename RegisterHelper>
        using deduce_return_t = decltype(OpTag::apply(
            std::declval<this_arg_t<typename RegisterHelper::class_type, ThisConst>&>(),
            std::declval<std::add_lvalue_reference_t<Rhs>>()
        ));
    };

    // Case 3: param OP this (LHS is arbitrary type, RHS is class type)
    // Only for binary operators, not assignment
    template <typename OpTag, typename Lhs, bool AutoDecl, bool ThisConst>
    class binary_param_this_op :
        private param_placeholder<Lhs, AutoDecl>,
        public operator_base<binary_param_this_op<OpTag, Lhs, AutoDecl, ThisConst>, true>
    {
        using param_type = param_placeholder<Lhs, AutoDecl>;

    public:
        binary_param_this_op(const param_type& p) : param_type(p) {}

        using operator_base<binary_param_this_op, true>::operator();

        template <typename Return, typename RegisterHelper>
        void register_impl(RegisterHelper& ar, std::string_view ret_decl) const
        {
            using class_type = typename RegisterHelper::class_type;
            using R = this_arg_t<class_type, ThisConst>;

            // Reversed operator name: append "_r" suffix
            auto rev_name = string_concat(OpTag::name, "_r");

            auto decl = detail::gen_binary_param_decl<AutoDecl, Lhs>(
                ret_decl, rev_name, this->param_type::get_decl(), ThisConst
            );

            ar.method(
                decl,
                [](Lhs l, R& r) -> Return
                { return OpTag::apply(l, r); },
                objlast
            );
        }

        template <typename RegisterHelper>
        using deduce_return_t = decltype(OpTag::apply(
            std::declval<std::add_lvalue_reference_t<Lhs>>(),
            std::declval<this_arg_t<typename RegisterHelper::class_type, ThisConst>&>()
        ));
    };

    // opCmp — always returns int, no return_proxy_with_decl

    // Case 1: this <=> this
    template <bool LhsConst, bool RhsConst>
    class cmp_this_this_op : public operator_base<cmp_this_this_op<LhsConst, RhsConst>, false>
    {
    public:
        template <typename Return, typename RegisterHelper>
        void register_impl(RegisterHelper& ar, std::string_view ret_decl) const
        {
            static_assert(std::same_as<Return, int>, "opCmp(_r) only returns int");
            using class_type = typename RegisterHelper::class_type;
            using L = this_arg_t<class_type, LhsConst>;
            using R = this_arg_t<class_type, RhsConst>;

            auto decl = detail::gen_binary_auto_decl</*Ref=*/true, RhsConst>(
                ret_decl, op_cmp::name, ar.get_name(), LhsConst
            );

            ar.method(
                decl,
                [](L& l, R& r) -> int
                { return op_cmp::apply(l, r); },
                objfirst
            );
        }

        template <typename RegisterHelper>
        using deduce_return_t = int;
    };

    // Case 2: this <=> param
    template <bool ThisConst, typename Rhs, bool AutoDecl>
    class cmp_this_param_op :
        private param_placeholder<Rhs, AutoDecl>,
        public operator_base<cmp_this_param_op<ThisConst, Rhs, AutoDecl>, false>
    {
        using param_type = param_placeholder<Rhs, AutoDecl>;

    public:
        cmp_this_param_op(const param_type& p) : param_type(p) {}

        using operator_base<cmp_this_param_op, false>::operator();

        template <typename Return, typename RegisterHelper>
        void register_impl(RegisterHelper& ar, std::string_view) const
        {
            static_assert(std::same_as<Return, int>, "opCmp(_r) only returns int");
            using class_type = typename RegisterHelper::class_type;
            using L = this_arg_t<class_type, ThisConst>;

            auto decl = detail::gen_binary_param_decl<AutoDecl, Rhs>(
                "int", op_cmp::name, this->param_type::get_decl(), ThisConst
            );

            ar.method(
                decl,
                [](L& l, Rhs r) -> int
                { return op_cmp::apply(l, r); },
                objfirst
            );
        }

        template <typename RegisterHelper>
        using deduce_return_t = int;
    };

    // Case 3: param <=> this
    template <typename Lhs, bool AutoDecl, bool ThisConst>
    class cmp_param_this_op :
        private param_placeholder<Lhs, AutoDecl>,
        public operator_base<cmp_param_this_op<Lhs, AutoDecl, ThisConst>, false>
    {
        using param_type = param_placeholder<Lhs, AutoDecl>;

    public:
        cmp_param_this_op(const param_type& p) : param_type(p) {}

        using operator_base<cmp_param_this_op, false>::operator();

        template <typename Return, typename RegisterHelper>
        void register_impl(RegisterHelper& ar, std::string_view) const
        {
            static_assert(std::same_as<Return, int>, "opCmp(_r) only returns int");
            using class_type = typename RegisterHelper::class_type;
            using R = this_arg_t<class_type, ThisConst>;

            auto decl = detail::gen_binary_param_decl<AutoDecl, Lhs>(
                "int", string_concat(op_cmp::name, "_r"), this->param_type::get_decl(), ThisConst
            );

            ar.method(
                decl,
                [](Lhs l, R& r) -> int
                { return op_cmp::apply(l, r); },
                objlast
            );
        }

        template <typename RegisterHelper>
        using deduce_return_t = int;
    };

    // Index operator class templates

    // Case 1: this[this] — index by class type
    template <bool ThisConst, bool IndexConst>
    class index_this_op : public operator_base<index_this_op<ThisConst, IndexConst>, true>
    {
    public:
        template <typename Return, typename RegisterHelper>
        void register_impl(RegisterHelper& ar, std::string_view ret_decl) const
        {
            using class_type = typename RegisterHelper::class_type;
            using T = this_arg_t<class_type, ThisConst>;
            using I = this_arg_t<class_type, IndexConst>;

            auto decl = detail::gen_index_auto_decl</*Ref=*/true, IndexConst>(
                ret_decl, ar.get_name(), ThisConst
            );

            ar.method(
                decl,
                [](T& t, I& i) -> Return
                { return t[i]; },
                objfirst
            );
        }

        template <typename RegisterHelper>
        using deduce_return_t = decltype(std::declval<this_arg_t<typename RegisterHelper::class_type, ThisConst>&>()
                                             [std::declval<this_arg_t<typename RegisterHelper::class_type, IndexConst>&>()]);
    };

    // Case 2: this[param] — index by arbitrary type
    template <bool ThisConst, typename Index, bool AutoDecl>
    class index_param_op :
        private param_placeholder<Index, AutoDecl>,
        public operator_base<index_param_op<ThisConst, Index, AutoDecl>, true>
    {
        using param_type = param_placeholder<Index, AutoDecl>;

    public:
        index_param_op(const param_type& p) : param_type(p) {}

        using operator_base<index_param_op, true>::operator();

        template <typename Return, typename RegisterHelper>
        void register_impl(RegisterHelper& ar, std::string_view ret_decl) const
        {
            using class_type = typename RegisterHelper::class_type;
            using T = this_arg_t<class_type, ThisConst>;

            auto decl = detail::gen_index_param_decl<AutoDecl, Index>(
                ret_decl, this->param_type::get_decl(), ThisConst
            );

            ar.method(
                decl,
                [](T& t, Index i) -> Return
                { return t[i]; }
            );
        }

        template <typename RegisterHelper>
        using deduce_return_t = decltype(std::declval<this_arg_t<typename RegisterHelper::class_type, ThisConst>&>()
                                             [std::declval<std::add_lvalue_reference_t<Index>>()]);
    };

    // Unary operator class templates

    // Prefix unary: OP this  (e.g., -this, ++this, ~this)
    template <typename OpTag, bool ThisConst>
    class unary_prefix_op : public operator_base<unary_prefix_op<OpTag, ThisConst>, true>
    {
    public:
        template <typename Return, typename RegisterHelper>
        void register_impl(RegisterHelper& ar, std::string_view ret_decl) const
        {
            using class_type = typename RegisterHelper::class_type;
            using T = this_arg_t<class_type, ThisConst>;

            auto decl = detail::gen_unary_decl(ret_decl, OpTag::name, ThisConst);

            ar.method(
                decl,
                [](T& v) -> Return
                { return OpTag::apply(v); },
                objlast
            );
        }

        template <typename RegisterHelper>
        using deduce_return_t = decltype(OpTag::apply(
            std::declval<this_arg_t<typename RegisterHelper::class_type, ThisConst>&>()
        ));
    };

    // Suffix unary: this OP  (e.g., this++, this--)
    template <typename OpTag, bool ThisConst>
    class unary_suffix_op : public operator_base<unary_suffix_op<OpTag, ThisConst>, true>
    {
    public:
        template <typename Return, typename RegisterHelper>
        void register_impl(RegisterHelper& ar, std::string_view ret_decl) const
        {
            using class_type = typename RegisterHelper::class_type;
            using T = this_arg_t<class_type, ThisConst>;

            auto decl = detail::gen_unary_decl(ret_decl, OpTag::name, ThisConst);

            ar.method(
                decl,
                [](T& v) -> Return
                { return OpTag::apply(v); },
                objlast
            );
        }

        template <typename RegisterHelper>
        using deduce_return_t = decltype(OpTag::apply(
            std::declval<this_arg_t<typename RegisterHelper::class_type, ThisConst>&>()
        ));
    };

} // namespace detail

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

    // Assignment operators: this OP param
#define ASBIND20_THIS_ASSIGN_OP(cpp_op, OpTag)                              \
    template <typename Rhs, bool AutoDecl>                                  \
    auto operator cpp_op(const param_placeholder<Rhs, AutoDecl>& rhs) const \
        ->detail::binary_this_param_op<OpTag, IsConst, Rhs, AutoDecl>       \
    {                                                                       \
        return {rhs};                                                       \
    }

    ASBIND20_THIS_ASSIGN_OP(+=, detail::opAddAssign)
    ASBIND20_THIS_ASSIGN_OP(-=, detail::opSubAssign)
    ASBIND20_THIS_ASSIGN_OP(*=, detail::opMulAssign)
    ASBIND20_THIS_ASSIGN_OP(/=, detail::opDivAssign)
    ASBIND20_THIS_ASSIGN_OP(%=, detail::opModAssign)
    ASBIND20_THIS_ASSIGN_OP(&=, detail::opAndAssign)
    ASBIND20_THIS_ASSIGN_OP(|=, detail::opOrAssign)
    ASBIND20_THIS_ASSIGN_OP(^=, detail::opXorAssign)
    ASBIND20_THIS_ASSIGN_OP(<<=, detail::opShlAssign)
    ASBIND20_THIS_ASSIGN_OP(>>=, detail::opShrAssign)

#undef ASBIND20_THIS_ASSIGN_OP

    // Index operators
    template <bool IndexConst>
    auto operator[](this_placeholder<IndexConst>) const
        -> detail::index_this_op<IsConst, IndexConst>
    {
        return {};
    }

    template <typename Index, bool AutoDecl>
    auto operator[](const param_placeholder<Index, AutoDecl>& idx) const
        -> detail::index_param_op<IsConst, Index, AutoDecl>
    {
        return {idx};
    }
};

inline constexpr this_placeholder<false> _this;
inline constexpr this_placeholder<true> const_this;

// Free function operator overloads

// Unary prefix operators
template <bool ThisConst>
constexpr auto operator-(this_placeholder<ThisConst>)
    -> detail::unary_prefix_op<detail::opNeg, ThisConst>
{
    return {};
}

template <bool ThisConst>
constexpr auto operator~(this_placeholder<ThisConst>)
    -> detail::unary_prefix_op<detail::opCom, ThisConst>
{
    return {};
}

template <bool ThisConst>
constexpr auto operator++(this_placeholder<ThisConst>)
    -> detail::unary_prefix_op<detail::opPreInc, ThisConst>
{
    return {};
}

template <bool ThisConst>
constexpr auto operator--(this_placeholder<ThisConst>)
    -> detail::unary_prefix_op<detail::opPreDec, ThisConst>
{
    return {};
}

// Unary suffix operators (extra int parameter is C++ postfix convention)
template <bool ThisConst>
constexpr auto operator++(this_placeholder<ThisConst>, int)
    -> detail::unary_suffix_op<detail::opPostInc, ThisConst>
{
    return {};
}

template <bool ThisConst>
constexpr auto operator--(this_placeholder<ThisConst>, int)
    -> detail::unary_suffix_op<detail::opPostDec, ThisConst>
{
    return {};
}

// Binary operator overloads: this OP this, this OP param, param OP this
#define ASBIND20_BINARY_OVERLOADS(cpp_op, OpTag)                                                        \
    template <bool LhsConst, bool RhsConst>                                                             \
    constexpr auto operator cpp_op(this_placeholder<LhsConst>, this_placeholder<RhsConst>)              \
        ->detail::binary_this_this_op<OpTag, LhsConst, RhsConst>                                        \
    {                                                                                                   \
        return {};                                                                                      \
    }                                                                                                   \
    template <bool ThisConst, typename T2, bool Auto>                                                   \
    constexpr auto operator cpp_op(this_placeholder<ThisConst>, const param_placeholder<T2, Auto>& rhs) \
        ->detail::binary_this_param_op<OpTag, ThisConst, T2, Auto>                                      \
    {                                                                                                   \
        return {rhs};                                                                                   \
    }                                                                                                   \
    template <typename T1, bool Auto, bool ThisConst>                                                   \
    constexpr auto operator cpp_op(const param_placeholder<T1, Auto>& lhs, this_placeholder<ThisConst>) \
        ->detail::binary_param_this_op<OpTag, T1, Auto, ThisConst>                                      \
    {                                                                                                   \
        return {lhs};                                                                                   \
    }

ASBIND20_BINARY_OVERLOADS(<=>, detail::op_cmp)
ASBIND20_BINARY_OVERLOADS(+, detail::opAdd)
ASBIND20_BINARY_OVERLOADS(-, detail::opSub)
ASBIND20_BINARY_OVERLOADS(*, detail::opMul)
ASBIND20_BINARY_OVERLOADS(/, detail::opDiv)
ASBIND20_BINARY_OVERLOADS(%, detail::opMod)
ASBIND20_BINARY_OVERLOADS(&, detail::opAnd)
ASBIND20_BINARY_OVERLOADS(|, detail::opOr)
ASBIND20_BINARY_OVERLOADS(^, detail::opXor)
ASBIND20_BINARY_OVERLOADS(<<, detail::opShl)
ASBIND20_BINARY_OVERLOADS(>>, detail::opShr)

#undef ASBIND20_BINARY_OVERLOADS

} // namespace asbind20

#endif
