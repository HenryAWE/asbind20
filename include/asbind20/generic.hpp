/**
 * @file generic.hpp
 * @author HenryAWE
 * @brief Generic wrapper
 */

#ifndef ASBIND20_GENERIC_HPP
#define ASBIND20_GENERIC_HPP

#pragma once

#include <cstddef>
#include <bit>
#include <array>
#include <algorithm>
#include <functional>
#include "detail/include_as.hpp"
#include "meta.hpp"
#include "utility.hpp"
#include "type_traits.hpp"

namespace asbind20
{
namespace detail
{
    template <typename T>
    concept is_native_function_helper = std::is_function_v<T> ||
                                        std::is_function_v<std::remove_pointer_t<T>> ||
                                        std::is_member_function_pointer_v<T>;
} // namespace detail

/**
 * @brief Acceptable native function for AngelScript
 */
template <typename T>
concept native_function =
    !std::is_convertible_v<T, AS_NAMESPACE_QUALIFIER asGENFUNC_t> &&
    detail::is_native_function_helper<std::decay_t<T>>;

/**
 * @brief Lambda without any captured values
 *
 * @note This concept will also reject the function for generic calling convention
 */
template <typename Lambda>
concept noncapturing_lambda = requires() {
    { +Lambda{} } -> native_function;
} && std::is_empty_v<Lambda>;

template <native_function auto Function>
struct fp_wrapper_t
{
    static constexpr auto get() noexcept
    {
        return Function;
    }
};

/**
 * @brief Wrap NTTP function pointer as type
 *
 * @tparam Function NTTP function pointer
 */
template <native_function auto Function>
constexpr inline fp_wrapper_t<Function> fp{};

template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
struct call_conv_t
{};

/**
 * @brief Helper for specifying calling convention
 */
template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
constexpr inline call_conv_t<CallConv> call_conv;

constexpr inline call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_GENERIC> generic_call_conv{};

template <typename T>
class auxiliary_wrapper
{
public:
    auxiliary_wrapper() = delete;
    constexpr auxiliary_wrapper(const auxiliary_wrapper&) noexcept = default;

    explicit constexpr auxiliary_wrapper(T* aux) noexcept
        : m_aux(aux) {}

    [[nodiscard]]
    void* get_address() const noexcept
    {
        return (void*)m_aux;
    }

private:
    T* m_aux;
};

template <>
class auxiliary_wrapper<this_type_t>
{
public:
    auxiliary_wrapper() = delete;
    constexpr auxiliary_wrapper(const auxiliary_wrapper&) noexcept = default;

    explicit constexpr auxiliary_wrapper(this_type_t) noexcept {};
};

template <typename T>
[[nodiscard]]
constexpr auxiliary_wrapper<T> auxiliary(T& aux) noexcept
{
    return auxiliary_wrapper<T>(std::addressof(aux));
}

template <typename T>
[[nodiscard]]
constexpr auxiliary_wrapper<T> auxiliary(T* aux) noexcept
{
    return auxiliary_wrapper<T>(aux);
}

[[nodiscard]]
constexpr inline auxiliary_wrapper<void> auxiliary(std::nullptr_t) noexcept
{
    return auxiliary_wrapper<void>(nullptr);
}

[[nodiscard]]
constexpr inline auxiliary_wrapper<this_type_t> auxiliary(this_type_t) noexcept
{
    return auxiliary_wrapper<this_type_t>(this_type);
}

template <typename T>
constexpr auxiliary_wrapper<T> auxiliary(T&& aux) = delete;

/**
 * @brief Store a pointer-sized integer value as auxiliary object
 *
 * @note DO NOT use this unless you know what you are doing!
 *
 * @warning Only use this with the @b generic calling convention!
 *
 * @param val Integer value
 */
[[nodiscard]]
constexpr auxiliary_wrapper<void> aux_value(std::intptr_t val) noexcept
{
    return auxiliary_wrapper<void>(std::bit_cast<void*>(val));
}

template <std::size_t... Is>
struct var_type_t : public std::index_sequence<Is...>
{};

/**
 * @brief Helper for specifying position of variable type parameters
 *
 * @tparam Is Position of variable type arguments in the script function declaration
 */
template <std::size_t... Is>
inline constexpr var_type_t<Is...> var_type{};

class composite_wrapper
{
public:
    using composite_wrapper_tag = void;

    composite_wrapper() = delete;
    constexpr composite_wrapper(const composite_wrapper&) noexcept = default;

    constexpr explicit composite_wrapper(std::size_t off) noexcept
        : m_off(off) {}

    constexpr composite_wrapper& operator=(const composite_wrapper&) noexcept = default;

    [[nodiscard]]
    constexpr std::size_t get_offset() const noexcept
    {
        return m_off;
    }

private:
    std::size_t m_off;
};

template <auto Composite>
class composite_wrapper_nontype;

template <auto MemberObject>
requires(std::is_member_object_pointer_v<decltype(MemberObject)>)
class composite_wrapper_nontype<MemberObject>
{
public:
    using composite_wrapper_tag = void;

    operator composite_wrapper() const noexcept
    {
        // The implementation of member_offset is not constexpr-friendly,
        // so this function should not be constexpr.
        return composite_wrapper(member_offset(MemberObject));
    }
};

template <std::size_t Offset>
class composite_wrapper_nontype<Offset>
{
public:
    using composite_wrapper_tag = void;

    constexpr operator composite_wrapper() const noexcept
    {
        return composite_wrapper(Offset);
    }
};

constexpr composite_wrapper composite(std::size_t off) noexcept
{
    return composite_wrapper(off);
}

template <typename MemberPointer>
requires(std::is_member_object_pointer_v<MemberPointer>)
composite_wrapper composite(MemberPointer mp) noexcept
{
    return composite_wrapper(member_offset(mp));
}

template <auto Composite>
constexpr composite_wrapper_nontype<Composite> composite() noexcept
{
    return {};
}

namespace detail
{
    template <std::size_t RawArgCount, std::size_t... Is>
    consteval auto gen_script_arg_idx(var_type_t<Is...> = {})
    {
        // Generate an index of script argument.
        // For example, given var_type<1> and
        // RawArgCount = 4, which can be (float, void*, int, float) in C++,
        // the result should be {0, 1, 1, 2}, which means (float, ?&in, float) in the AS.

        static_assert(RawArgCount >= sizeof...(Is), "Invalid argument count");

        constexpr std::size_t script_arg_count = RawArgCount - sizeof...(Is);
        constexpr std::size_t var_type_pos[]{Is...};

        std::array<std::size_t, RawArgCount> tmp{}; // result
        std::size_t current_arg_pos = 0;
        std::size_t j = 0; // index for tmp
        std::size_t k = 0; // index for var_type_pos
        for(std::size_t i = 0; i < script_arg_count; ++i)
        {
            if(k < sizeof...(Is) && i == var_type_pos[k])
            {
                ++k;
                tmp[j++] = current_arg_pos;
                tmp[j++] = current_arg_pos++;
                continue;
            }

            tmp[j++] = current_arg_pos++;
        }

        return tmp;
    }
} // namespace detail

#ifdef _MSC_VER
#    pragma warning(push)
/*
* When binding functions that may directly throw an exception,
* MSVC will report warning of unreachable code.
*/
#    pragma warning(disable : 4702)
#endif

/**
 * @brief Get pointer/reference to the object
 *
 * @tparam T Object type, can be a pointer, otherwise the return type will a reference
 */
template <typename T>
auto get_generic_object(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
    -> std::conditional_t<std::is_pointer_v<T>, T, std::add_lvalue_reference_t<T>>
{
    void* obj = gen->GetObject();
    if constexpr(std::is_pointer_v<T>)
    {
        return (T)obj;
    }
    else
    {
        using pointer_t = std::add_pointer_t<std::remove_reference_t<T>>;
        return *(pointer_t)obj;
    }
}

template <typename T>
auto get_generic_auxiliary(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
    -> std::conditional_t<std::is_pointer_v<T>, T, std::add_lvalue_reference_t<T>>
{
    void* obj = gen->GetAuxiliary();
    if constexpr(std::is_pointer_v<T>)
    {
        return (T)obj;
    }
    else
    {
        using pointer_t = std::add_pointer_t<std::remove_reference_t<T>>;
        return *(pointer_t)obj;
    }
}

template <typename T>
T get_generic_arg(
    AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen,
    AS_NAMESPACE_QUALIFIER asUINT idx
)
{
    constexpr bool is_customized = requires() {
        { type_traits<std::remove_cv_t<T>>::get_arg(gen, idx) } -> std::convertible_to<T>;
    };

    if constexpr(is_customized)
    {
        return type_traits<std::remove_cv_t<T>>::get_arg(gen, idx);
    }
    else if constexpr(std::is_pointer_v<T>)
    {
        using value_t = std::remove_cv_t<std::remove_pointer_t<T>>;

        if constexpr(std::same_as<value_t, AS_NAMESPACE_QUALIFIER asIScriptObject>)
        {
            void* ptr = gen->GetArgObject(idx);
            return static_cast<AS_NAMESPACE_QUALIFIER asIScriptObject*>(ptr);
        }
        else if constexpr(std::same_as<value_t, AS_NAMESPACE_QUALIFIER asITypeInfo>)
        {
            return *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(idx);
        }
        else if constexpr(std::same_as<value_t, AS_NAMESPACE_QUALIFIER asIScriptEngine>)
        {
            return *(AS_NAMESPACE_QUALIFIER asIScriptEngine**)gen->GetAddressOfArg(idx);
        }
        else
        {
            void* ptr = gen->GetArgAddress(idx);
            return (T)ptr;
        }
    }
    else if constexpr(std::is_reference_v<T>)
    {
        using pointer_t = std::remove_reference_t<T>*;
        return *get_generic_arg<pointer_t>(gen, idx);
    }
    else if constexpr(std::is_class_v<T>)
    {
        using pointer_t = std::remove_reference_t<T>*;
        return std::move(*(pointer_t)gen->GetArgObject(idx));
    }
    else if constexpr(std::is_enum_v<T>)
    {
        return static_cast<T>(get_generic_arg<int>(gen, idx));
    }
    else if constexpr(std::integral<T>)
    {
        if constexpr(sizeof(T) == sizeof(AS_NAMESPACE_QUALIFIER asBYTE))
            return static_cast<T>(gen->GetArgByte(idx));
        else if constexpr(sizeof(T) == sizeof(AS_NAMESPACE_QUALIFIER asWORD))
            return static_cast<T>(gen->GetArgWord(idx));
        else if constexpr(sizeof(T) == sizeof(AS_NAMESPACE_QUALIFIER asDWORD))
            return static_cast<T>(gen->GetArgDWord(idx));
        else if constexpr(sizeof(T) == sizeof(AS_NAMESPACE_QUALIFIER asQWORD))
            return static_cast<T>(gen->GetArgQWord(idx));
        else
            static_assert(!sizeof(T), "Integer size too large");
    }
    else if constexpr(std::floating_point<T>)
    {
        if constexpr(std::same_as<std::remove_cv_t<T>, float>)
            return gen->GetArgFloat(idx);
        else if constexpr(std::same_as<std::remove_cv_t<T>, double>)
            return gen->GetArgDouble(idx);
        else
            static_assert(!sizeof(T), "Unsupported floating point type");
    }
    else
    {
        static_assert(!sizeof(T), "Unsupported type");
    }
}

template <typename Return>
void set_generic_return(
    AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen,
    std::type_identity_t<Return>&& ret
)
{
    constexpr bool is_customized = requires() {
        { type_traits<std::remove_cv_t<Return>>::set_return(gen, std::forward<Return>(ret)) } -> std::same_as<int>;
    };

    if constexpr(is_customized)
    {
        type_traits<std::remove_cv_t<Return>>::set_return(gen, std::forward<Return>(ret));
    }
    else if constexpr(std::is_reference_v<Return>)
    {
        using pointer_t = std::remove_reference_t<Return>*;
        set_generic_return<pointer_t>(gen, std::addressof(ret));
    }
    else if constexpr(std::is_pointer_v<Return>)
    {
        void* ptr = (void*)ret;

        if constexpr(
            std::same_as<std::remove_cv_t<Return>, AS_NAMESPACE_QUALIFIER asIScriptObject*> ||
            std::same_as<std::remove_cv_t<Return>, const AS_NAMESPACE_QUALIFIER asIScriptObject*>
        )
            gen->SetReturnObject(ptr);
        else
        {
            gen->SetReturnAddress(ptr);
        }
    }
    else if constexpr(std::is_class_v<Return>)
    {
        if constexpr(meta::is_constructible_at_v<Return, Return&&>)
        {
            void* mem = gen->GetAddressOfReturnLocation();
            new(mem) Return(std::forward<Return>(ret));
        }
        else
        {
            gen->SetReturnObject((void*)std::addressof(ret));
        }
    }
    else if constexpr(std::is_enum_v<Return>)
    {
        set_generic_return<int>(gen, static_cast<int>(ret));
    }
    else if constexpr(std::integral<Return>)
    {
        if constexpr(sizeof(Return) == sizeof(AS_NAMESPACE_QUALIFIER asBYTE))
            gen->SetReturnByte(static_cast<AS_NAMESPACE_QUALIFIER asBYTE>(ret));
        else if constexpr(sizeof(Return) == sizeof(AS_NAMESPACE_QUALIFIER asWORD))
            gen->SetReturnWord(static_cast<AS_NAMESPACE_QUALIFIER asWORD>(ret));
        else if constexpr(sizeof(Return) == sizeof(AS_NAMESPACE_QUALIFIER asDWORD))
            gen->SetReturnDWord(static_cast<AS_NAMESPACE_QUALIFIER asDWORD>(ret));
        else if constexpr(sizeof(Return) == sizeof(AS_NAMESPACE_QUALIFIER asQWORD))
            gen->SetReturnQWord(static_cast<AS_NAMESPACE_QUALIFIER asQWORD>(ret));
        else // Compiler extension like __int128
            new(gen->GetAddressOfReturnLocation()) Return(ret);
    }
    else if constexpr(std::floating_point<Return>)
    {
        if constexpr(std::same_as<std::remove_cv_t<Return>, float>)
            gen->SetReturnFloat(ret);
        else if constexpr(std::same_as<std::remove_cv_t<Return>, double>)
            gen->SetReturnDouble(ret);
        else
            static_assert(!sizeof(Return), "Unsupported floating point type");
    }
    else
    {
        static_assert(!sizeof(Return), "Unsupported type");
    }
}

/**
 * @brief Set generic return value by invocation result
 *
 * @note Use this function instead of directly using the `set_generic_return` for non-moveable types.
 *       This function will try to use NRVO for non-moveable types.
 *
 * @tparam Return Return type
 *
 * @param gen The interface for the generic calling convention
 * @param fn Function to invoke
 * @param args Arguments for the function
 */
template <typename Return, typename Fn, typename... Args>
void set_generic_return_by(
    AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen,
    Fn&& fn,
    Args&&... args
)
{
    constexpr bool is_customized = requires() {
        { type_traits<std::remove_cv_t<Return>>::set_return(gen, std::declval<Return>()) } -> std::same_as<int>;
    };
    constexpr bool non_moveable =
        !(std::is_reference_v<Return> || std::is_pointer_v<Return>) &&
        std::is_class_v<Return> &&
        (!is_customized ||
         std::is_move_constructible_v<Return>);

    if constexpr(std::is_void_v<Return>)
    {
        std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...);
    }
    else if constexpr(non_moveable) // Try NRVO for non-moveable types
    {
        void* mem = gen->GetAddressOfReturnLocation();
        new(mem) Return(std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...));
    }
    else
    {
        set_generic_return<Return>(
            gen,
            std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...)
        );
    }
}

/**
 * @brief Get the `this` pointer based on calling convention of wrapped function
 *
 * @tparam FunctionType The function signature of wrapped function
 * @tparam CallConv Calling convention of original function, must support `this` pointer
 *
 * @note For `asCALL_THISCALL_OBJFIRST/LAST`, this function will return `this` for the `OBJFIRST/LAST`.
 *       Please use the `get_generic_auxiliary` for the auxiliary pointer. @sa get_generic_auxiliary
 */
template <
    typename FunctionType,
    AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
decltype(auto) get_generic_this(
    AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen
)
{
    using traits = function_traits<FunctionType>;

    constexpr bool from_auxiliary =
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL;

    void* ptr = nullptr;
    if constexpr(from_auxiliary)
        ptr = gen->GetAuxiliary();
    else
        ptr = gen->GetObject();

    if constexpr(
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL ||
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL
    )
    {
        using pointer_t = typename traits::class_type*;
        return static_cast<pointer_t>(ptr);
    }
    else if constexpr(
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST ||
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_OBJFIRST
    )
    {
        using this_arg_t = typename traits::first_arg_type;
        if constexpr(std::is_pointer_v<this_arg_t>)
            return static_cast<this_arg_t>(ptr);
        else
            return *static_cast<std::remove_reference_t<this_arg_t>*>(ptr);
    }
    else if constexpr(
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST ||
        CallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_OBJLAST
    )
    {
        using this_arg_t = typename traits::last_arg_type;
        if constexpr(std::is_pointer_v<this_arg_t>)
            return static_cast<this_arg_t>(ptr);
        else
            return *static_cast<std::remove_reference_t<this_arg_t>*>(ptr);
    }
    else
        static_assert(!CallConv && false, "This calling convention doesn't have a this pointer");
}

namespace detail
{
    template <typename T> // unused
    int var_type_helper(
        std::true_type, AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen, std::size_t idx
    )
    {
        return gen->GetArgTypeId(static_cast<AS_NAMESPACE_QUALIFIER asUINT>(idx));
    }

    template <typename T>
    T var_type_helper(
        std::false_type, AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen, std::size_t idx
    )
    {
        return get_generic_arg<T>(gen, static_cast<AS_NAMESPACE_QUALIFIER asUINT>(idx));
    }

    template <std::size_t... Is>
    constexpr bool var_type_tag_helper(var_type_t<Is...>, std::size_t raw_idx)
    {
        // Plus 1 for the position of type id
        std::size_t arr[]{(Is + 1)...};

        // Returns true if the position is for type id
        return std::find(std::begin(arr), std::end(arr), raw_idx) != std::end(arr);
    }

    template <typename VarType, std::size_t RawIdx>
    using var_type_tag = std::bool_constant<var_type_tag_helper(VarType{}, RawIdx)>;

#define ASBIND20_GENERIC_WRAPPER_IMPL(func)                                         \
    static void wrapper_impl_thiscall(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) \
    {                                                                               \
        [gen]<std::size_t... Is>(std::index_sequence<Is...>)                        \
        {                                                                           \
            set_generic_return_by<typename traits::return_type>(                    \
                gen,                                                                \
                func,                                                               \
                this_(gen),                                                         \
                get_generic_arg<typename traits::template arg_type<Is>>(            \
                    gen, static_cast<AS_NAMESPACE_QUALIFIER asUINT>(Is)             \
                )...                                                                \
            );                                                                      \
        }(std::make_index_sequence<traits::arg_count::value>());                    \
    }                                                                               \
    static void wrapper_impl_objfirst(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) \
    {                                                                               \
        static_assert(traits::arg_count::value >= 1);                               \
        [gen]<std::size_t... Is>(std::index_sequence<Is...>)                        \
        {                                                                           \
            set_generic_return_by<typename traits::return_type>(                    \
                gen,                                                                \
                func,                                                               \
                this_(gen),                                                         \
                get_generic_arg<typename traits::template arg_type<Is + 1>>(        \
                    gen, static_cast<AS_NAMESPACE_QUALIFIER asUINT>(Is)             \
                )...                                                                \
            );                                                                      \
        }(std::make_index_sequence<traits::arg_count::value - 1>());                \
    }                                                                               \
    static void wrapper_impl_objlast(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)  \
    {                                                                               \
        static_assert(traits::arg_count::value >= 1);                               \
        [gen]<std::size_t... Is>(std::index_sequence<Is...>)                        \
        {                                                                           \
            set_generic_return_by<typename traits::return_type>(                    \
                gen,                                                                \
                func,                                                               \
                get_generic_arg<typename traits::template arg_type<Is>>(            \
                    gen, static_cast<AS_NAMESPACE_QUALIFIER asUINT>(Is)             \
                )...,                                                               \
                this_(gen)                                                          \
            );                                                                      \
        }(std::make_index_sequence<traits::arg_count::value - 1>());                \
    }                                                                               \
    static void wrapper_impl_general(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)  \
    {                                                                               \
        [gen]<std::size_t... Is>(std::index_sequence<Is...>)                        \
        {                                                                           \
            set_generic_return_by<typename traits::return_type>(                    \
                gen,                                                                \
                func,                                                               \
                get_generic_arg<typename traits::template arg_type<Is>>(            \
                    gen, static_cast<AS_NAMESPACE_QUALIFIER asUINT>(Is)             \
                )...                                                                \
            );                                                                      \
        }(std::make_index_sequence<traits::arg_count::value>());                    \
    }

#define ASBIND20_GENERIC_WRAPPER_VAR_TYPE_IMPL(func)                                                    \
    template <typename VarType>                                                                         \
    static void var_type_wrapper_impl_thiscall(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)            \
    {                                                                                                   \
        using traits = function_traits<function_type>;                                                  \
        static constexpr auto indices = detail::gen_script_arg_idx<traits::arg_count_v>(VarType{});     \
        [gen]<std::size_t... Is>(std::index_sequence<Is...>)                                            \
        {                                                                                               \
            set_generic_return_by<typename traits::return_type>(                                        \
                gen,                                                                                    \
                func,                                                                                   \
                this_(gen),                                                                             \
                detail::var_type_helper<typename traits::template arg_type<Is>>(                        \
                    detail::var_type_tag<VarType, Is>{},                                                \
                    gen,                                                                                \
                    static_cast<AS_NAMESPACE_QUALIFIER asUINT>(indices[Is])                             \
                )...                                                                                    \
            );                                                                                          \
        }(std::make_index_sequence<indices.size()>());                                                  \
    }                                                                                                   \
    template <typename VarType>                                                                         \
    static void var_type_wrapper_impl_objfirst(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)            \
    {                                                                                                   \
        using traits = function_traits<function_type>;                                                  \
        static constexpr auto indices = detail::gen_script_arg_idx<traits::arg_count_v - 1>(VarType{}); \
        [gen]<std::size_t... Is>(std::index_sequence<Is...>)                                            \
        {                                                                                               \
            set_generic_return_by<typename traits::return_type>(                                        \
                gen,                                                                                    \
                func,                                                                                   \
                this_(gen),                                                                             \
                detail::var_type_helper<typename traits::template arg_type<Is + 1>>(                    \
                    detail::var_type_tag<VarType, Is>{},                                                \
                    gen,                                                                                \
                    static_cast<AS_NAMESPACE_QUALIFIER asUINT>(indices[Is])                             \
                )...                                                                                    \
            );                                                                                          \
        }(std::make_index_sequence<indices.size()>());                                                  \
    }                                                                                                   \
    template <typename VarType>                                                                         \
    static void var_type_wrapper_impl_objlast(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)             \
    {                                                                                                   \
        using traits = function_traits<function_type>;                                                  \
        static constexpr auto indices = detail::gen_script_arg_idx<traits::arg_count_v - 1>(VarType{}); \
        [gen]<std::size_t... Is>(std::index_sequence<Is...>)                                            \
        {                                                                                               \
            set_generic_return_by<typename traits::return_type>(                                        \
                gen,                                                                                    \
                func,                                                                                   \
                detail::var_type_helper<typename traits::template arg_type<Is>>(                        \
                    detail::var_type_tag<VarType, Is>{},                                                \
                    gen,                                                                                \
                    static_cast<AS_NAMESPACE_QUALIFIER asUINT>(indices[Is])                             \
                )...,                                                                                   \
                this_(gen)                                                                              \
            );                                                                                          \
        }(std::make_index_sequence<indices.size()>());                                                  \
    }                                                                                                   \
    template <typename VarType>                                                                         \
    static void var_type_wrapper_impl_general(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)             \
    {                                                                                                   \
        using traits = function_traits<function_type>;                                                  \
        static constexpr auto indices = detail::gen_script_arg_idx<traits::arg_count_v>(VarType{});     \
        [gen]<std::size_t... Is>(std::index_sequence<Is...>)                                            \
        {                                                                                               \
            set_generic_return_by<typename traits::return_type>(                                        \
                gen,                                                                                    \
                func,                                                                                   \
                detail::var_type_helper<typename traits::template arg_type<Is>>(                        \
                    detail::var_type_tag<VarType, Is>{},                                                \
                    gen,                                                                                \
                    static_cast<AS_NAMESPACE_QUALIFIER asUINT>(indices[Is])                             \
                )...                                                                                    \
            );                                                                                          \
        }(std::make_index_sequence<indices.size()>());                                                  \
    }

    template <
        noncapturing_lambda Lambda,
        AS_NAMESPACE_QUALIFIER asECallConvTypes OriginalConv>
    requires(OriginalConv != AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
    class generic_wrapper_lambda
    {
    public:
        using function_type = decltype(+Lambda{});

    private:
        using traits = function_traits<function_type>;

        static decltype(auto) this_(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
        {
            return get_generic_this<function_type, OriginalConv>(gen);
        }

        ASBIND20_GENERIC_WRAPPER_IMPL(+Lambda{})
        ASBIND20_GENERIC_WRAPPER_VAR_TYPE_IMPL(+Lambda{})

    public:
        static constexpr auto generate() noexcept
            -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
        {
            if constexpr(
                OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL ||
                OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL
            )
                return &wrapper_impl_thiscall;
            else if constexpr(OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST)
                return &wrapper_impl_objfirst;
            else if constexpr(OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST)
                return &wrapper_impl_objlast;
            else
            {
                static_assert(
                    OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL ||
                    OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_STDCALL
                );
                return &wrapper_impl_general;
            }
        }

        template <std::size_t... Is>
        static constexpr auto generate(var_type_t<Is...>)
            -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
        {
            using my_var_type = var_type_t<Is...>;
            if constexpr(
                OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL ||
                OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL
            )
                return &var_type_wrapper_impl_thiscall<my_var_type>;
            else if constexpr(OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST)
                return &var_type_wrapper_impl_objfirst<my_var_type>;
            else if constexpr(OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST)
                return &var_type_wrapper_impl_objlast<my_var_type>;
            else
            {
                static_assert(
                    OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL ||
                    OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_STDCALL
                );
                return &var_type_wrapper_impl_general<my_var_type>;
            }
        }
    };

    template <
        native_function auto Function,
        AS_NAMESPACE_QUALIFIER asECallConvTypes OriginalConv>
    requires(OriginalConv != AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
    class generic_wrapper_nontype
    {
    public:
        using function_type = std::decay_t<decltype(Function)>;

        static constexpr auto underlying_convention() noexcept
            -> AS_NAMESPACE_QUALIFIER asECallConvTypes
        {
            return OriginalConv;
        }

    private:
        using traits = function_traits<function_type>;

        static decltype(auto) this_(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
        {
            return get_generic_this<function_type, OriginalConv>(gen);
        }

        // MSVC needs to directly use the Function as a workaround,
        // i.e., NO constexpr/consteval function for getting function pointer,
        // otherwise it may cause strange runtime behavior.
        ASBIND20_GENERIC_WRAPPER_IMPL(Function)
        ASBIND20_GENERIC_WRAPPER_VAR_TYPE_IMPL(Function)

        // THISCALL_OBJFIRST/LAST are only available for the member function pointer, i.e.,
        // no lambda support.

        static void wrapper_impl_thiscall_objfirst(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
        {
            static_assert(traits::arg_count::value >= 1);
            [gen]<std::size_t... Is>(std::index_sequence<Is...>)
            {
                set_generic_return_by<typename traits::return_type>(
                    gen,
                    Function,
                    get_generic_auxiliary<typename traits::class_type*>(gen),
                    this_(gen),
                    get_generic_arg<typename traits::template arg_type<Is + 1>>(
                        gen, static_cast<AS_NAMESPACE_QUALIFIER asUINT>(Is)
                    )...
                );
            }(std::make_index_sequence<traits::arg_count::value - 1>());
        }

        static void wrapper_impl_thiscall_objlast(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
        {
            static_assert(traits::arg_count::value >= 1);
            [gen]<std::size_t... Is>(std::index_sequence<Is...>)
            {
                set_generic_return_by<typename traits::return_type>(
                    gen,
                    Function,
                    get_generic_auxiliary<typename traits::class_type*>(gen),
                    get_generic_arg<typename traits::template arg_type<Is>>(
                        gen, static_cast<AS_NAMESPACE_QUALIFIER asUINT>(Is)
                    )...,
                    this_(gen)
                );
            }(std::make_index_sequence<traits::arg_count::value - 1>());
        }

        template <typename VarType>
        static void var_type_wrapper_impl_thiscall_objfirst(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
        {
            using traits = function_traits<function_type>;
            static constexpr auto indices = detail::gen_script_arg_idx<traits::arg_count_v - 1>(VarType{});
            [gen]<std::size_t... Is>(std::index_sequence<Is...>)
            {
                set_generic_return_by<typename traits::return_type>(
                    gen,
                    Function,
                    get_generic_auxiliary<typename traits::class_type*>(gen),
                    this_(gen),
                    detail::var_type_helper<typename traits::template arg_type<Is + 1>>(
                        detail::var_type_tag<VarType, Is>{},
                        gen,
                        static_cast<AS_NAMESPACE_QUALIFIER asUINT>(indices[Is])
                    )...
                );
            }(std::make_index_sequence<indices.size()>());
        }

        template <typename VarType>
        static void var_type_wrapper_impl_thiscall_objlast(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
        {
            using traits = function_traits<function_type>;
            static constexpr auto indices = detail::gen_script_arg_idx<traits::arg_count_v - 1>(VarType{});
            [gen]<std::size_t... Is>(std::index_sequence<Is...>)
            {
                set_generic_return_by<typename traits::return_type>(
                    gen,
                    Function,
                    get_generic_auxiliary<typename traits::class_type*>(gen),
                    detail::var_type_helper<typename traits::template arg_type<Is>>(
                        detail::var_type_tag<VarType, Is>{},
                        gen,
                        static_cast<AS_NAMESPACE_QUALIFIER asUINT>(indices[Is])
                    )...,
                    this_(gen)
                );
            }(std::make_index_sequence<indices.size()>());
        }

    public:
        static constexpr auto generate() noexcept
            -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
        {
            if constexpr(
                OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL ||
                OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL
            )
                return &wrapper_impl_thiscall;
            else if constexpr(OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST)
                return &wrapper_impl_objfirst;
            else if constexpr(OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST)
                return &wrapper_impl_objlast;
            else if constexpr(OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_OBJFIRST)
                return &wrapper_impl_thiscall_objfirst;
            else if constexpr(OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_OBJLAST)
                return &wrapper_impl_thiscall_objlast;
            else
                return &wrapper_impl_general;
        }

        template <std::size_t... Is>
        static constexpr auto generate(var_type_t<Is...>)
            -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
        {
            using my_var_type = var_type_t<Is...>;
            if constexpr(
                OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL ||
                OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL
            )
                return &var_type_wrapper_impl_thiscall<my_var_type>;
            else if constexpr(OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST)
                return &var_type_wrapper_impl_objfirst<my_var_type>;
            else if constexpr(OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST)
                return &var_type_wrapper_impl_objlast<my_var_type>;
            else if constexpr(OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_OBJFIRST)
                return &var_type_wrapper_impl_thiscall_objfirst<my_var_type>;
            else if constexpr(OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_OBJLAST)
                return &var_type_wrapper_impl_thiscall_objlast<my_var_type>;
            else
                return &var_type_wrapper_impl_general<my_var_type>;
        }
    };

#undef ASBIND20_GENERIC_WRAPPER_IMPL
#undef ASBIND20_GENERIC_WRAPPER_VAR_TYPE_IMPL

    template <
        noncapturing_lambda Lambda,
        AS_NAMESPACE_QUALIFIER asECallConvTypes OriginalCallConv,
        std::size_t... Is>
    consteval auto lambda_to_asGENFUNC_t_impl()
        -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
    {
#ifndef _MSC_VER
        if constexpr(sizeof...(Is))
            return generic_wrapper_lambda<Lambda, OriginalCallConv>::generate(var_type<Is...>);
        else
            return generic_wrapper_lambda<Lambda, OriginalCallConv>::generate();

#else
        // GCC < 13.2 has problem on this branch,
        // but MSVC has problem on the previous branch.
        // Clang supports both branches.

        if constexpr(sizeof...(Is))
            return generic_wrapper_nontype<+Lambda{}, OriginalCallConv>::generate(var_type<Is...>);
        else
            return generic_wrapper_nontype<+Lambda{}, OriginalCallConv>::generate();

#endif
    }

    template <
        native_function auto Function,
        AS_NAMESPACE_QUALIFIER asECallConvTypes OriginalCallConv,
        std::size_t... Is>
    constexpr auto fp_to_asGENFUNC_t_impl()
        -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
    {
        if constexpr(sizeof...(Is))
            return generic_wrapper_nontype<Function, OriginalCallConv>::generate(var_type<Is...>);
        else
            return generic_wrapper_nontype<Function, OriginalCallConv>::generate();
    }

    template <typename Class, auto Composite>
    struct comp_accessor;

    template <typename Class, std::size_t Offset>
    struct comp_accessor<Class, Offset>
    {
        static Class* get(void* base) noexcept
        {
            Class* ptr = *(Class**)((std::byte*)base + Offset);
            return ptr;
        }
    };

    template <typename Class, auto MemberObject>
    requires(std::is_member_object_pointer_v<decltype(MemberObject)>)
    struct comp_accessor<Class, MemberObject>
    {
        static Class* get(void* base) noexcept
        {
            std::size_t offset = member_offset(MemberObject);
            Class* ptr = *(Class**)((std::byte*)base + offset);
            return ptr;
        }
    };

    template <
        auto Function,
        typename CompositeWrapper>
    class generic_wrapper_composite;

    template <
        auto Function,
        auto Composite>
    class generic_wrapper_composite<Function, composite_wrapper_nontype<Composite>>
    {
    private:
        using function_type = std::decay_t<decltype(Function)>;
        using traits = function_traits<function_type>;

        static decltype(auto) this_(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
        {
            using class_type = std::conditional_t<
                traits::is_const_v,
                std::add_const_t<typename traits::class_type>,
                typename traits::class_type>;

            return comp_accessor<class_type, Composite>::get(gen->GetObject());
        }

        static void wrapper_comp(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
        {
            [gen]<std::size_t... Is>(std::index_sequence<Is...>)
            {
                set_generic_return_by<typename traits::return_type>(
                    gen,
                    Function,
                    this_(gen),
                    get_generic_arg<typename traits::template arg_type<Is>>(
                        gen, static_cast<AS_NAMESPACE_QUALIFIER asUINT>(Is)
                    )...
                );
            }(std::make_index_sequence<traits::arg_count::value>());
        }

        template <typename VarType>
        static void var_type_wrapper_comp(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
        {
            static constexpr auto indices = detail::gen_script_arg_idx<traits::arg_count_v>(VarType{});
            [gen]<std::size_t... Is>(std::index_sequence<Is...>)
            {
                set_generic_return_by<typename traits::return_type>(
                    gen,
                    Function,
                    this_(gen),
                    detail::var_type_helper<typename traits::template arg_type<Is>>(
                        detail::var_type_tag<VarType, Is>{},
                        gen,
                        static_cast<AS_NAMESPACE_QUALIFIER asUINT>(indices[Is])
                    )...
                );
            }(std::make_index_sequence<indices.size()>());
        }

    public:
        static constexpr auto generate() noexcept
            -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
        {
            return &wrapper_comp;
        }

        template <std::size_t... Is>
        static constexpr auto generate(var_type_t<Is...>) noexcept
            -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
        {
            using my_var_type = var_type_t<Is...>;
            return &var_type_wrapper_comp<my_var_type>;
        }
    };

    template <
        native_function auto Function,
        auto Composite,
        std::size_t... Is>
    constexpr auto fp_to_asGENFUNC_t_impl_comp()
        -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
    {
        if constexpr(sizeof...(Is))
            return generic_wrapper_composite<Function, composite_wrapper_nontype<Composite>>::generate(var_type<Is...>);
        else
            return generic_wrapper_composite<Function, composite_wrapper_nontype<Composite>>::generate();
    }
} // namespace detail

#ifdef _MSC_VER
#    pragma warning(pop)
#endif

template <
    noncapturing_lambda Lambda,
    AS_NAMESPACE_QUALIFIER asECallConvTypes OriginalCallConv>
requires(OriginalCallConv != AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
consteval auto to_asGENFUNC_t(const Lambda&, call_conv_t<OriginalCallConv>)
    -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
{
    return detail::lambda_to_asGENFUNC_t_impl<Lambda, OriginalCallConv>();
}

template <
    native_function auto Function,
    AS_NAMESPACE_QUALIFIER asECallConvTypes OriginalCallConv>
requires(OriginalCallConv != AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
consteval auto to_asGENFUNC_t(fp_wrapper_t<Function>, call_conv_t<OriginalCallConv>)
    -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
{
    return detail::fp_to_asGENFUNC_t_impl<Function, OriginalCallConv>();
}

template <
    noncapturing_lambda Lambda,
    AS_NAMESPACE_QUALIFIER asECallConvTypes OriginalCallConv,
    std::size_t... Is>
consteval auto to_asGENFUNC_t(const Lambda&, call_conv_t<OriginalCallConv>, var_type_t<Is...>)
    -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
{
    return detail::lambda_to_asGENFUNC_t_impl<Lambda, OriginalCallConv, Is...>();
}

template <
    native_function auto Function,
    AS_NAMESPACE_QUALIFIER asECallConvTypes OriginalCallConv,
    std::size_t... Is>
consteval auto to_asGENFUNC_t(fp_wrapper_t<Function>, call_conv_t<OriginalCallConv>, var_type_t<Is...>)
    -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
{
    return detail::fp_to_asGENFUNC_t_impl<Function, OriginalCallConv, Is...>();
}

template <
    native_function auto Function,
    auto Composite>
consteval auto to_asGENFUNC_t(
    fp_wrapper_t<Function>,
    call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_THISCALL>, // Calling convention parameter reserved for the future
    composite_wrapper_nontype<Composite>
)
    -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
{
    return detail::fp_to_asGENFUNC_t_impl_comp<Function, Composite>();
}

template <
    native_function auto Function,
    auto Composite,
    std::size_t... Is>
consteval auto to_asGENFUNC_t(
    fp_wrapper_t<Function>,
    call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_THISCALL>, // Calling convention parameter reserved for the future
    composite_wrapper_nontype<Composite>,
    var_type_t<Is...>
)
    -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
{
    return detail::fp_to_asGENFUNC_t_impl_comp<Function, Composite, Is...>();
}

template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
requires(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
constexpr auto to_asGENFUNC_t(AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn, call_conv_t<CallConv>)
    -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
{
    return gfn;
}

namespace detail
{
    template <
        native_function auto ConstructorFunc,
        typename Class,
        bool Template,
        AS_NAMESPACE_QUALIFIER asECallConvTypes OriginalCallConv>
    class generic_wrapper_ctor_func
    {
    public:
        using native_function_type = std::decay_t<decltype(ConstructorFunc)>;

        static auto generate() noexcept
            -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
        {
            using traits = function_traits<decltype(ConstructorFunc)>;
            using args_tuple = typename traits::args_tuple;

            if constexpr(OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST)
            {
                return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                {
                    if constexpr(Template)
                    {
                        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                        {
                            Class* mem = (Class*)gen->GetObject();
                            std::invoke(
                                ConstructorFunc,
                                mem,
                                *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0),
                                get_generic_arg<std::tuple_element_t<Is + 2, args_tuple>>(
                                    gen, (AS_NAMESPACE_QUALIFIER asUINT)Is + 1
                                )...
                            );
                        }(std::make_index_sequence<traits::arg_count_v - 2>());
                    }
                    else
                    {
                        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                        {
                            Class* mem = (Class*)gen->GetObject();
                            std::invoke(
                                ConstructorFunc,
                                mem,
                                get_generic_arg<std::tuple_element_t<Is + 1, args_tuple>>(
                                    gen, (AS_NAMESPACE_QUALIFIER asUINT)Is
                                )...
                            );
                        }(std::make_index_sequence<traits::arg_count_v - 1>());
                    }
                };
            }
            else // OriginalCallConv == asCALL_CDECL_OBJLAST
            {
                return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                {
                    if constexpr(Template)
                    {
                        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                        {
                            Class* mem = (Class*)gen->GetObject();
                            std::invoke(
                                ConstructorFunc,
                                *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0),
                                get_generic_arg<std::tuple_element_t<Is + 1, args_tuple>>(
                                    gen, (AS_NAMESPACE_QUALIFIER asUINT)Is + 1
                                )...,
                                mem
                            );
                        }(std::make_index_sequence<traits::arg_count_v - 2>());
                    }
                    else
                    {
                        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                        {
                            Class* mem = (Class*)gen->GetObject();
                            std::invoke(
                                ConstructorFunc,
                                get_generic_arg<std::tuple_element_t<Is, args_tuple>>(
                                    gen, (AS_NAMESPACE_QUALIFIER asUINT)Is
                                )...,
                                mem
                            );
                        }(std::make_index_sequence<traits::arg_count_v - 1>());
                    }
                };
            }
        }
    };

    template <
        typename ConstructorLambda,
        typename Class,
        bool Template,
        AS_NAMESPACE_QUALIFIER asECallConvTypes OriginalCallConv>
    class generic_wrapper_ctor_lambda
    {
    public:
        using native_function_type = std::decay_t<decltype(+ConstructorLambda{})>;

        static auto generate() noexcept
            -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
        {
            using traits = function_traits<native_function_type>;
            using args_tuple = typename traits::args_tuple;

            if constexpr(OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST)
            {
                return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                {
                    if constexpr(Template)
                    {
                        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                        {
                            Class* mem = (Class*)gen->GetObject();
                            std::invoke(
                                ConstructorLambda{},
                                mem,
                                *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0),
                                get_generic_arg<std::tuple_element_t<Is + 2, args_tuple>>(
                                    gen, (AS_NAMESPACE_QUALIFIER asUINT)Is + 1
                                )...
                            );
                        }(std::make_index_sequence<traits::arg_count_v - 2>());
                    }
                    else
                    {
                        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                        {
                            Class* mem = (Class*)gen->GetObject();
                            std::invoke(
                                ConstructorLambda{},
                                mem,
                                get_generic_arg<std::tuple_element_t<Is + 1, args_tuple>>(
                                    gen, (AS_NAMESPACE_QUALIFIER asUINT)Is
                                )...
                            );
                        }(std::make_index_sequence<traits::arg_count_v - 1>());
                    }
                };
            }
            else // OriginalCallConv == asCALL_CDECL_OBJLAST
            {
                return +[](AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) -> void
                {
                    if constexpr(Template)
                    {
                        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                        {
                            Class* mem = (Class*)gen->GetObject();
                            std::invoke(
                                ConstructorLambda{},
                                *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0),
                                get_generic_arg<std::tuple_element_t<Is + 1, args_tuple>>(
                                    gen, (AS_NAMESPACE_QUALIFIER asUINT)Is + 1
                                )...,
                                mem
                            );
                        }(std::make_index_sequence<traits::arg_count_v - 2>());
                    }
                    else
                    {
                        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
                        {
                            Class* mem = (Class*)gen->GetObject();
                            std::invoke(
                                ConstructorLambda{},
                                get_generic_arg<std::tuple_element_t<Is, args_tuple>>(
                                    gen, (AS_NAMESPACE_QUALIFIER asUINT)Is
                                )...,
                                mem
                            );
                        }(std::make_index_sequence<traits::arg_count_v - 1>());
                    }
                };
            }
        }
    };
} // namespace detail

template <
    typename Class,
    bool IsTemplate,
    native_function auto ConstructorFunc,
    asECallConvTypes CallConv>
constexpr auto constructor_to_asGENFUNC_t(
    fp_wrapper_t<ConstructorFunc>,
    call_conv_t<CallConv>
)
{
    using gen_t = detail::generic_wrapper_ctor_func<
        ConstructorFunc,
        Class,
        IsTemplate,
        CallConv>;
    return gen_t::generate();
}

template <
    typename Class,
    bool IsTemplate,
    noncapturing_lambda ConstructorLambda,
    asECallConvTypes CallConv>
constexpr auto constructor_to_asGENFUNC_t(
    const ConstructorLambda&,
    call_conv_t<CallConv>
)
{
    using gen_t = detail::generic_wrapper_ctor_lambda<
        ConstructorLambda,
        Class,
        IsTemplate,
        CallConv>;
    return gen_t::generate();
}
} // namespace asbind20

#endif
