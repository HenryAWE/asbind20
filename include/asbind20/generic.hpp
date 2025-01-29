#ifndef ASBIND20_GENERIC_HPP
#define ASBIND20_GENERIC_HPP

#pragma once

#include <array>
#include <algorithm>
#include "detail/include_as.hpp" // IWYU pragma: keep
#include "utility.hpp"
#include "type_traits.hpp"

namespace asbind20
{
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
        // RawArgCount = 5, which can be (float, void*, int, float) in C++,
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

template <typename T>
auto get_generic_object(asIScriptGeneric* gen)
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
T get_generic_arg(asIScriptGeneric* gen, asUINT idx)
{
    constexpr bool is_customized = requires() {
        { type_traits<std::remove_cv_t<T>>::get_arg(gen, idx) } -> std::convertible_to<T>;
    };

    if constexpr(is_customized)
    {
        return type_traits<std::remove_cv_t<T>>::get_arg(gen, idx);
    }
    else if constexpr(
        std::is_same_v<std::remove_cv_t<T>, asITypeInfo*> ||
        std::is_same_v<std::remove_cv_t<T>, const asITypeInfo*>
    )
    {
        return *(asITypeInfo**)gen->GetAddressOfArg(idx);
    }
    else if constexpr(
        std::is_same_v<std::remove_cv_t<T>, asIScriptObject*> ||
        std::is_same_v<std::remove_cv_t<T>, const asIScriptObject*>
    )
    {
        return static_cast<asIScriptObject*>(gen->GetArgObject(idx));
    }
    else if constexpr(std::is_pointer_v<T>)
    {
        using value_t = std::remove_pointer_t<T>;

        void* ptr = nullptr;
        if constexpr(std::is_class_v<value_t> || std::is_void_v<value_t>)
            ptr = gen->GetArgAddress(idx);
        else
            ptr = gen->GetAddressOfArg(idx);
        assert(ptr != nullptr);
        return (T)ptr;
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
        if constexpr(sizeof(T) == sizeof(asBYTE))
            return static_cast<T>(gen->GetArgByte(idx));
        else if constexpr(sizeof(T) == sizeof(asWORD))
            return static_cast<T>(gen->GetArgWord(idx));
        else if constexpr(sizeof(T) == sizeof(asDWORD))
            return static_cast<T>(gen->GetArgDWord(idx));
        else if constexpr(sizeof(T) == sizeof(asQWORD))
            return static_cast<T>(gen->GetArgQWord(idx));
        else
            static_assert(!sizeof(T), "Integer size too large");
    }
    else if constexpr(std::floating_point<T>)
    {
        if constexpr(std::is_same_v<std::remove_cv_t<T>, float>)
            return gen->GetArgFloat(idx);
        else if constexpr(std::is_same_v<std::remove_cv_t<T>, double>)
            return gen->GetArgDouble(idx);
        else
            static_assert(!sizeof(T), "Unsupported floating point type");
    }
    else
    {
        static_assert(!sizeof(T), "Unsupported type");
    }
}

template <typename T>
void set_generic_return(asIScriptGeneric* gen, T&& ret)
{
    constexpr bool is_customized = requires() {
        { type_traits<std::remove_cv_t<T>>::set_return(gen, std::forward<T>(ret)) } -> std::same_as<int>;
    };

    if constexpr(is_customized)
    {
        type_traits<std::remove_cv_t<T>>::set_return(gen, std::forward<T>(ret));
    }
    else if constexpr(std::is_reference_v<T>)
    {
        using pointer_t = std::remove_reference_t<T>*;
        set_generic_return<pointer_t>(gen, std::addressof(ret));
    }
    else if constexpr(std::is_pointer_v<T>)
    {
        void* ptr = (void*)ret;

        if constexpr(
            std::is_same_v<std::remove_cv_t<T>, asIScriptObject*> ||
            std::is_same_v<std::remove_cv_t<T>, const asIScriptObject*>
        )
            gen->SetReturnObject(ptr);
        else
        {
            gen->SetReturnAddress(ptr);
        }
    }
    else if constexpr(std::is_class_v<T>)
    {
        void* mem = gen->GetAddressOfReturnLocation();
        new(mem) T(std::forward<T>(ret));
    }
    else if constexpr(std::is_enum_v<T>)
    {
        set_generic_return<int>(gen, static_cast<int>(ret));
    }
    else if constexpr(std::integral<T>)
    {
        if constexpr(sizeof(T) == sizeof(asBYTE))
            gen->SetReturnByte(static_cast<asBYTE>(ret));
        else if constexpr(sizeof(T) == sizeof(asWORD))
            gen->SetReturnWord(static_cast<asWORD>(ret));
        else if constexpr(sizeof(T) == sizeof(asDWORD))
            gen->SetReturnDWord(static_cast<asDWORD>(ret));
        else if constexpr(sizeof(T) == sizeof(asQWORD))
            gen->SetReturnQWord(static_cast<asQWORD>(ret));
        else
            static_assert(!sizeof(T), "Integer size too large");
    }
    else if constexpr(std::floating_point<T>)
    {
        if constexpr(std::is_same_v<std::remove_cv_t<T>, float>)
            gen->SetReturnFloat(ret);
        else if constexpr(std::is_same_v<std::remove_cv_t<T>, double>)
            gen->SetReturnDouble(ret);
        else
            static_assert(!sizeof(T), "Unsupported floating point type");
    }
    else
    {
        static_assert(!sizeof(T), "Unsupported type");
    }
}

namespace detail
{
    template <typename T> // unused
    int var_type_helper(std::true_type, asIScriptGeneric* gen, std::size_t idx)
    {
        return gen->GetArgTypeId(static_cast<asUINT>(idx));
    }

    template <typename T>
    T var_type_helper(std::false_type, asIScriptGeneric* gen, std::size_t idx)
    {
        return get_generic_arg<T>(gen, static_cast<asUINT>(idx));
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
} // namespace detail

template <
    native_function auto Function,
    asECallConvTypes OriginalConv>
requires(OriginalConv != asCALL_GENERIC)
class generic_wrapper_t
{
public:
    using function_type = std::decay_t<decltype(Function)>;

    static_assert(
        !std::is_member_function_pointer_v<function_type> || OriginalConv == asCALL_THISCALL,
        "Invalid calling convention"
    );

    constexpr generic_wrapper_t() noexcept = default;

    constexpr generic_wrapper_t(const generic_wrapper_t&) noexcept = default;

    static constexpr function_type underlying_function() noexcept
    {
        return Function;
    }

    static constexpr asECallConvTypes underlying_convention() noexcept
    {
        return OriginalConv;
    }

    static constexpr asGENFUNC_t generate() noexcept
    {
        if constexpr(OriginalConv == asCALL_THISCALL)
            return &wrapper_impl_thiscall;
        else if constexpr(OriginalConv == asCALL_CDECL_OBJFIRST)
            return &wrapper_impl_objfirst;
        else if constexpr(OriginalConv == asCALL_CDECL_OBJLAST)
            return &wrapper_impl_objlast;
        else
            return &wrapper_impl_general;
    }

    template <std::size_t... Is>
    static constexpr asGENFUNC_t generate(var_type_t<Is...>)
    {
        using my_var_type = var_type_t<Is...>;
        if constexpr(OriginalConv == asCALL_THISCALL)
            return &var_type_wrapper_impl_thiscall<my_var_type>;
        else if constexpr(OriginalConv == asCALL_CDECL_OBJFIRST)
            return &var_type_wrapper_impl_objfirst<my_var_type>;
        else if constexpr(OriginalConv == asCALL_CDECL_OBJLAST)
            return &var_type_wrapper_impl_objlast<my_var_type>;
        else
            return &var_type_wrapper_impl_general<my_var_type>;
    }

    constexpr asGENFUNC_t operator()() const noexcept
    {
        return generate();
    }

    template <std::size_t... Is>
    constexpr asGENFUNC_t operator()(var_type_t<Is...>) const noexcept
    {
        return generate(var_type<Is...>);
    }

    static decltype(auto) get_generic_this(asIScriptGeneric* gen)
    {
        using traits = function_traits<function_type>;

        void* ptr = gen->GetObject();

        if constexpr(OriginalConv == asCALL_THISCALL)
        {
            using pointer_t = typename traits::class_type*;
            return static_cast<pointer_t>(ptr);
        }
        else if constexpr(OriginalConv == asCALL_CDECL_OBJFIRST)
        {
            using this_arg_t = typename traits::first_arg_type;
            if constexpr(std::is_pointer_v<this_arg_t>)
                return static_cast<this_arg_t>(ptr);
            else
                return *static_cast<std::remove_reference_t<this_arg_t>*>(ptr);
        }
        else if constexpr(OriginalConv == asCALL_CDECL_OBJLAST)
        {
            using this_arg_t = typename traits::last_arg_type;
            if constexpr(std::is_pointer_v<this_arg_t>)
                return static_cast<this_arg_t>(ptr);
            else
                return *static_cast<std::remove_reference_t<this_arg_t>*>(ptr);
        }
        else
            static_assert(!OriginalConv && false, "This calling convention doesn't have a this pointer");
    }

public:
    using traits = function_traits<function_type>;

    static void wrapper_impl_thiscall(asIScriptGeneric* gen)
    {
        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            if constexpr(std::is_void_v<typename traits::return_type>)
            {
                std::invoke(
                    Function,
                    get_generic_this(gen),
                    get_generic_arg<typename traits::template arg_type<Is>>(
                        gen, static_cast<asUINT>(Is)
                    )...
                );
            }
            else
            {
                set_generic_return<typename traits::return_type>(
                    gen,
                    std::invoke(
                        Function,
                        get_generic_this(gen),
                        get_generic_arg<typename traits::template arg_type<Is>>(
                            gen, static_cast<asUINT>(Is)
                        )...
                    )
                );
            }
        }(std::make_index_sequence<traits::arg_count::value>());
    }

    static void wrapper_impl_objfirst(asIScriptGeneric* gen)
    {
        static_assert(traits::arg_count::value >= 1);

        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            if constexpr(std::is_void_v<typename traits::return_type>)
            {
                std::invoke(
                    Function,
                    get_generic_this(gen),
                    get_generic_arg<typename traits::template arg_type<Is + 1>>(
                        gen, static_cast<asUINT>(Is)
                    )...
                );
            }
            else
            {
                set_generic_return<typename traits::return_type>(
                    gen,
                    std::invoke(
                        Function,
                        get_generic_this(gen),
                        get_generic_arg<typename traits::template arg_type<Is + 1>>(
                            gen, static_cast<asUINT>(Is)
                        )...
                    )
                );
            }
        }(std::make_index_sequence<traits::arg_count::value - 1>());
    }

    static void wrapper_impl_objlast(asIScriptGeneric* gen)
    {
        static_assert(traits::arg_count::value >= 1);

        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            if constexpr(std::is_void_v<typename traits::return_type>)
            {
                std::invoke(
                    Function,
                    get_generic_arg<typename traits::template arg_type<Is>>(
                        gen, static_cast<asUINT>(Is)
                    )...,
                    get_generic_this(gen)
                );
            }
            else
            {
                set_generic_return<typename traits::return_type>(
                    gen,
                    std::invoke(
                        Function,
                        get_generic_arg<typename traits::template arg_type<Is>>(
                            gen, static_cast<asUINT>(Is)
                        )...,
                        get_generic_this(gen)
                    )
                );
            }
        }(std::make_index_sequence<traits::arg_count::value - 1>());
    }

    static void wrapper_impl_general(asIScriptGeneric* gen)
    {
        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            if constexpr(std::is_void_v<typename traits::return_type>)
            {
                std::invoke(
                    Function,
                    get_generic_arg<typename traits::template arg_type<Is>>(
                        gen, static_cast<asUINT>(Is)
                    )...
                );
            }
            else
            {
                set_generic_return<typename traits::return_type>(
                    gen,
                    std::invoke(
                        Function,
                        get_generic_arg<typename traits::template arg_type<Is>>(
                            gen, static_cast<asUINT>(Is)
                        )...
                    )
                );
            }
        }(std::make_index_sequence<traits::arg_count::value>());
    }

    template <typename VarType>
    static void var_type_wrapper_impl_thiscall(asIScriptGeneric* gen)
    {
        using traits = function_traits<function_type>;
        static constexpr auto indices = detail::gen_script_arg_idx<traits::arg_count_v>(VarType{});

        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            if constexpr(std::is_void_v<typename traits::return_type>)
            {
                std::invoke(
                    Function,
                    get_generic_this(gen),
                    detail::var_type_helper<typename traits::template arg_type<Is>>(
                        detail::var_type_tag<VarType, Is>{},
                        gen,
                        static_cast<asUINT>(indices[Is])
                    )...
                );
            }
            else
            {
                set_generic_return(
                    gen,
                    std::invoke(
                        Function,
                        get_generic_this(gen),
                        detail::var_type_helper<typename traits::template arg_type<Is>>(
                            detail::var_type_tag<VarType, Is>{},
                            gen,
                            static_cast<asUINT>(indices[Is])
                        )...
                    )
                );
            }
        }(std::make_index_sequence<indices.size()>());
    }

    template <typename VarType>
    static void var_type_wrapper_impl_objfirst(asIScriptGeneric* gen)
    {
        using traits = function_traits<function_type>;
        static constexpr auto indices = detail::gen_script_arg_idx<traits::arg_count_v - 1>(VarType{});

        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            if constexpr(std::is_void_v<typename traits::return_type>)
            {
                std::invoke(
                    Function,
                    get_generic_this(gen),
                    detail::var_type_helper<typename traits::template arg_type<Is + 1>>(
                        detail::var_type_tag<VarType, Is>{},
                        gen,
                        static_cast<asUINT>(indices[Is])
                    )...
                );
            }
            else
            {
                set_generic_return(
                    gen,
                    std::invoke(
                        Function,
                        get_generic_this(gen),
                        detail::var_type_helper<typename traits::template arg_type<Is + 1>>(
                            detail::var_type_tag<VarType, Is>{},
                            gen,
                            static_cast<asUINT>(indices[Is])
                        )...
                    )
                );
            }
        }(std::make_index_sequence<indices.size()>());
    }

    template <typename VarType>
    static void var_type_wrapper_impl_objlast(asIScriptGeneric* gen)
    {
        using traits = function_traits<function_type>;
        static constexpr auto indices = detail::gen_script_arg_idx<traits::arg_count_v - 1>(VarType{});

        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            if constexpr(std::is_void_v<typename traits::return_type>)
            {
                std::invoke(
                    Function,
                    detail::var_type_helper<typename traits::template arg_type<Is>>(
                        detail::var_type_tag<VarType, Is>{},
                        gen,
                        static_cast<asUINT>(indices[Is])
                    )...,
                    get_generic_this(gen)
                );
            }
            else
            {
                set_generic_return(
                    gen,
                    std::invoke(
                        Function,

                        detail::var_type_helper<typename traits::template arg_type<Is>>(
                            detail::var_type_tag<VarType, Is>{},
                            gen,
                            static_cast<asUINT>(indices[Is])
                        )...,
                        get_generic_this(gen)
                    )
                );
            }
        }(std::make_index_sequence<indices.size()>());
    }

    template <typename VarType>
    static void var_type_wrapper_impl_general(asIScriptGeneric* gen)
    {
        using traits = function_traits<function_type>;
        static constexpr auto indices = detail::gen_script_arg_idx<traits::arg_count_v>(VarType{});

        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            if constexpr(std::is_void_v<typename traits::return_type>)
            {
                std::invoke(
                    Function,
                    detail::var_type_helper<typename traits::template arg_type<Is>>(
                        detail::var_type_tag<VarType, Is>{},
                        gen,
                        static_cast<asUINT>(indices[Is])
                    )...
                );
            }
            else
            {
                set_generic_return(
                    gen,
                    std::invoke(
                        Function,
                        detail::var_type_helper<typename traits::template arg_type<Is>>(
                            detail::var_type_tag<VarType, Is>{},
                            gen,
                            static_cast<asUINT>(indices[Is])
                        )...
                    )
                );
            }
        }(std::make_index_sequence<indices.size()>());
    }
};

template <
    native_function auto Function,
    asECallConvTypes OriginalConv>
constexpr inline generic_wrapper_t<Function, OriginalConv> generic_wrapper{};
} // namespace asbind20

#endif
