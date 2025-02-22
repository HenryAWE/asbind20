#ifndef ASBIND20_GENERIC_HPP
#define ASBIND20_GENERIC_HPP

#pragma once

#include <array>
#include <algorithm>
#include "detail/include_as.hpp"
#include "utility.hpp"
#include "type_traits.hpp"

namespace asbind20
{
template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
struct call_conv_t
{};

template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
constexpr inline call_conv_t<CallConv> call_conv;

constexpr inline call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_GENERIC> generic_call_conv{};

template <std::size_t... Is>
struct var_type_t : public std::index_sequence<Is...>
{};

template <std::size_t... Is>
inline constexpr var_type_t<Is...> var_type{};

namespace detail
{
    template <std::size_t RawArgCount, std::size_t... Is>
    consteval auto gen_script_arg_idx(var_type_t<Is...> = {})
    {
        // Generate a index of script argument.
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
            return static_cast<asIScriptObject*>(ptr);
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
        return get_generic_arg<T&>(gen, idx);
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
            std::same_as<std::remove_cv_t<Return>, asIScriptObject*> ||
            std::same_as<std::remove_cv_t<Return>, const asIScriptObject*>
        )
            gen->SetReturnObject(ptr);
        else
        {
            gen->SetReturnAddress(ptr);
        }
    }
    else if constexpr(std::is_class_v<Return>)
    {
        void* mem = gen->GetAddressOfReturnLocation();
        new(mem) Return(std::forward<Return>(ret));
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
        else
            static_assert(!sizeof(Return), "Integer size too large");
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
 * @tparam Return Return type
 *
 * @param fn Function to invoke
 */
template <typename Return, typename Fn, typename... Args>
void set_generic_return_by(
    AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen,
    Fn&& fn,
    Args&&... args
)
{
    if constexpr(std::is_void_v<Return>)
    {
        std::invoke(std::forward<Fn>(fn), std::forward<Args>(args)...);
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

        static consteval function_type underlying_function()
        {
            return +Lambda{};
        }

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

        ASBIND20_GENERIC_WRAPPER_IMPL(underlying_function())
        ASBIND20_GENERIC_WRAPPER_VAR_TYPE_IMPL(underlying_function())

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
} // namespace detail

template <
    noncapturing_lambda Lambda,
    AS_NAMESPACE_QUALIFIER asECallConvTypes OriginalCallConv>
requires(OriginalCallConv != asCALL_GENERIC)
consteval auto to_asGENFUNC_t(const Lambda&, call_conv_t<OriginalCallConv>)
    -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
{
    return detail::lambda_to_asGENFUNC_t_impl<Lambda, OriginalCallConv>();
}

template <
    native_function auto Function,
    AS_NAMESPACE_QUALIFIER asECallConvTypes OriginalCallConv>
requires(OriginalCallConv != asCALL_GENERIC)
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

template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
requires(CallConv == AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
constexpr auto to_asGENFUNC_t(AS_NAMESPACE_QUALIFIER asGENFUNC_t gfn, call_conv_t<CallConv>)
    -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
{
    return gfn;
}
} // namespace asbind20

#endif
