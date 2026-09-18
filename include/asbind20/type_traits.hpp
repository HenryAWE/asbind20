/**
 * @file type_traits.hpp
 * @author HenryAWE
 * @brief Type traits for special conversion rules
 */

#ifndef ASBIND20_TYPE_TRAITS_HPP
#define ASBIND20_TYPE_TRAITS_HPP

#pragma once

#include "fwd.hpp"
#include "memory.hpp"

namespace asbind20
{
template <typename T>
struct type_traits
{};

// Forward declarations
// The following template functions are implemented in generic.hpp and invoke.hpp

template <typename T>
T get_generic_arg(
    generic_pointer gen,
    arg_index_type idx
);

template <typename Return>
int set_generic_return(
    generic_pointer gen,
    std::type_identity_t<Return>&& ret
);

template <typename T>
requires(!std::is_const_v<T> && !std::is_volatile_v<T>)
decltype(auto) get_script_return(context_reference ctx);

template <std::integral T>
int set_script_arg(
    context_reference ctx,
    arg_index_type idx,
    const T& val
);

/**
 * @brief Utility for quickly implementing type traits for enum with custom underlying type
 */
template <typename Enum>
requires std::is_enum_v<Enum>
struct underlying_enum_traits
{
    using underlying_type = std::underlying_type_t<Enum>;

    static int set_arg(
        context_reference ctx,
        arg_index_type arg,
        Enum val
    )
    {
        return set_script_arg(ctx, arg, static_cast<underlying_type>(val));
    }

    static Enum get_arg(
        generic_pointer gen,
        arg_index_type arg
    )
    {
        return static_cast<Enum>(get_generic_arg<underlying_type>(gen, arg));
    }

    static int set_return(
        generic_pointer gen, Enum val
    )
    {
        return set_generic_return<underlying_type>(gen, static_cast<underlying_type>(val));
    }

    static Enum get_return(
        context_reference ctx
    )
    {
        return static_cast<Enum>(get_script_return<underlying_type>(ctx));
    }
};

template <>
struct type_traits<std::byte> : public underlying_enum_traits<std::byte>
{};

template <>
struct type_traits<script_object>
{
    static int set_arg(
        context_reference ctx,
        arg_index_type arg,
        const script_object& val
    )
    {
        return ctx.SetArgObject(arg, val.get());
    }

    static script_object get_arg(
        generic_pointer gen,
        arg_index_type arg
    )
    {
        return script_object(
            static_cast<object_pointer>(gen->GetArgObject(arg))
        );
    }

    static int set_return(
        generic_pointer gen, const script_object& val
    )
    {
        return gen->SetReturnObject(val.get());
    }

    static script_object get_return(
        context_reference ctx
    )
    {
        return script_object(
            static_cast<object_pointer>(ctx.GetReturnObject())
        );
    }
};

namespace detail
{
    template <typename T>
    class type_traits_helper
    {
    public:
        using traits = asbind20::type_traits<T>;

    private:
        static constexpr bool has_customized_arg_setter_ref =
            requires(context_reference ctx, arg_index_type idx, T&& obj) {
                { traits::set_arg(ctx, idx, std::forward<T>(obj)) } -> std::convertible_to<int>;
            };

    public:
        static constexpr bool has_customized_arg_setter =
            has_customized_arg_setter_ref ||
            requires(context_pointer ctx, arg_index_type idx, T&& obj) {
                { traits::set_arg(ctx, idx, std::forward<T>(obj)) } -> std::convertible_to<int>;
            };

        template <typename Arg>
        static int set_arg(
            context_reference ctx,
            arg_index_type idx,
            Arg&& val
        )
        {
            if constexpr(has_customized_arg_setter_ref)
                return traits::set_arg(ctx, idx, std::forward<Arg>(val));
            else
                return traits::set_arg(std::addressof(ctx), idx, std::forward<Arg>(val));
        }

    private:
        static constexpr bool has_customized_ret_getter_ref =
            requires(context_reference ctx) {
                traits::get_return(ctx);
            };

    public:
        static constexpr bool has_customized_ret_getter =
            has_customized_ret_getter_ref ||
            requires(context_pointer ctx) {
                traits::get_return(ctx);
            };

        static decltype(auto) get_return(context_reference ctx)
        {
            if constexpr(has_customized_ret_getter_ref)
                return traits::get_return(ctx);
            else
                return traits::get_return(std::addressof(ctx));
        }

    private:
        static constexpr bool has_customized_ret_setter_ref =
            requires(generic_reference gen, T&& obj) {
                { traits::set_return(gen, std::forward<T>(obj)) } -> std::convertible_to<int>;
            };

    public:
        static constexpr bool has_customized_ret_setter =
            has_customized_ret_setter_ref ||
            requires(generic_pointer gen, T&& obj) {
                { traits::set_return(gen, std::forward<T>(obj)) } -> std::convertible_to<int>;
            };

        template <typename Return>
        static int set_return(
            generic_reference gen, Return&& val
        )
        {
            if constexpr(has_customized_ret_setter_ref)
                return traits::set_return(gen, std::forward<Return>(val));
            else
                return traits::set_return(std::addressof(gen), std::forward<Return>(val));
        }

    private:
        static constexpr bool has_customized_arg_getter_ref =
            requires(generic_reference gen, arg_index_type idx) {
                traits::get_arg(gen, idx);
            };

    public:
        static constexpr bool has_customized_arg_getter =
            has_customized_arg_getter_ref ||
            requires(generic_pointer gen, arg_index_type idx) {
                traits::get_arg(gen, idx);
            };

        static decltype(auto) get_arg(
            generic_reference gen, arg_index_type idx
        )
        {
            if constexpr(has_customized_arg_getter_ref)
                return traits::get_arg(gen, idx);
            else
                return traits::get_arg(std::addressof(gen), idx);
        }
    };

    // Workaround for older compiler like Clang 15
    // It will complain about "cannot form a reference to 'void'" for requires
    template <typename Void>
    requires(std::is_void_v<Void>)
    class type_traits_helper<Void>
    {
    public:
        using traits = asbind20::type_traits<void>;

        static constexpr bool has_customized_arg_setter = false;
        static constexpr bool has_customized_ret_getter = false;
        static constexpr bool has_customized_ret_setter = false;
        static constexpr bool has_customized_arg_getter = false;
    };
} // namespace detail
} // namespace asbind20

#endif
