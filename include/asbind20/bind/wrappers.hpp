/**
 * @file bind/wrappers.hpp
 * @author HenryAWE
 * @brief Generator of generic wrappers
 */

#ifndef ASBIND20_DETAIL_WRAPPERS_HPP
#define ASBIND20_DETAIL_WRAPPERS_HPP

#pragma once

#include <cstddef>
#include <array>
#include "../meta.hpp"
#include "../utility.hpp"
#include "../generic.hpp"

namespace asbind20::detail
{ // All contents in this file should NOT be directly used by user code

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

#define ASBIND20_GENERIC_WRAPPER_IMPL(func)                                    \
    static void wrapper_thiscall(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) \
    {                                                                          \
        [gen]<std::size_t... Is>(std::index_sequence<Is...>)                   \
        {                                                                      \
            set_generic_return_by<typename traits::return_type>(               \
                gen,                                                           \
                func,                                                          \
                this_(gen),                                                    \
                get_generic_arg<typename traits::template arg_type<Is>>(       \
                    gen, static_cast<AS_NAMESPACE_QUALIFIER asUINT>(Is)        \
                )...                                                           \
            );                                                                 \
        }(std::make_index_sequence<traits::arg_count::value>());               \
    }                                                                          \
    static void wrapper_objfirst(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen) \
    {                                                                          \
        static_assert(traits::arg_count::value >= 1);                          \
        [gen]<std::size_t... Is>(std::index_sequence<Is...>)                   \
        {                                                                      \
            set_generic_return_by<typename traits::return_type>(               \
                gen,                                                           \
                func,                                                          \
                this_(gen),                                                    \
                get_generic_arg<typename traits::template arg_type<Is + 1>>(   \
                    gen, static_cast<AS_NAMESPACE_QUALIFIER asUINT>(Is)        \
                )...                                                           \
            );                                                                 \
        }(std::make_index_sequence<traits::arg_count::value - 1>());           \
    }                                                                          \
    static void wrapper_objlast(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)  \
    {                                                                          \
        static_assert(traits::arg_count::value >= 1);                          \
        [gen]<std::size_t... Is>(std::index_sequence<Is...>)                   \
        {                                                                      \
            set_generic_return_by<typename traits::return_type>(               \
                gen,                                                           \
                func,                                                          \
                get_generic_arg<typename traits::template arg_type<Is>>(       \
                    gen, static_cast<AS_NAMESPACE_QUALIFIER asUINT>(Is)        \
                )...,                                                          \
                this_(gen)                                                     \
            );                                                                 \
        }(std::make_index_sequence<traits::arg_count::value - 1>());           \
    }                                                                          \
    static void wrapper_general(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)  \
    {                                                                          \
        [gen]<std::size_t... Is>(std::index_sequence<Is...>)                   \
        {                                                                      \
            set_generic_return_by<typename traits::return_type>(               \
                gen,                                                           \
                func,                                                          \
                get_generic_arg<typename traits::template arg_type<Is>>(       \
                    gen, static_cast<AS_NAMESPACE_QUALIFIER asUINT>(Is)        \
                )...                                                           \
            );                                                                 \
        }(std::make_index_sequence<traits::arg_count::value>());               \
    }

#define ASBIND20_GENERIC_WRAPPER_VAR_TYPE_IMPL(func)                                            \
    template <typename VarType>                                                                 \
    static void var_type_wrapper_thiscall(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)         \
    {                                                                                           \
        using traits = function_traits<function_type>;                                          \
        static constexpr auto indices = gen_script_arg_idx<traits::arg_count_v>(VarType{});     \
        [gen]<std::size_t... Is>(std::index_sequence<Is...>)                                    \
        {                                                                                       \
            set_generic_return_by<typename traits::return_type>(                                \
                gen,                                                                            \
                func,                                                                           \
                this_(gen),                                                                     \
                var_type_helper<typename traits::template arg_type<Is>>(                        \
                    var_type_tag<VarType, Is>{},                                                \
                    gen,                                                                        \
                    static_cast<AS_NAMESPACE_QUALIFIER asUINT>(indices[Is])                     \
                )...                                                                            \
            );                                                                                  \
        }(std::make_index_sequence<indices.size()>());                                          \
    }                                                                                           \
    template <typename VarType>                                                                 \
    static void var_type_wrapper_objfirst(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)         \
    {                                                                                           \
        using traits = function_traits<function_type>;                                          \
        static constexpr auto indices = gen_script_arg_idx<traits::arg_count_v - 1>(VarType{}); \
        [gen]<std::size_t... Is>(std::index_sequence<Is...>)                                    \
        {                                                                                       \
            set_generic_return_by<typename traits::return_type>(                                \
                gen,                                                                            \
                func,                                                                           \
                this_(gen),                                                                     \
                var_type_helper<typename traits::template arg_type<Is + 1>>(                    \
                    var_type_tag<VarType, Is>{},                                                \
                    gen,                                                                        \
                    static_cast<AS_NAMESPACE_QUALIFIER asUINT>(indices[Is])                     \
                )...                                                                            \
            );                                                                                  \
        }(std::make_index_sequence<indices.size()>());                                          \
    }                                                                                           \
    template <typename VarType>                                                                 \
    static void var_type_wrapper_objlast(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)          \
    {                                                                                           \
        using traits = function_traits<function_type>;                                          \
        static constexpr auto indices = gen_script_arg_idx<traits::arg_count_v - 1>(VarType{}); \
        [gen]<std::size_t... Is>(std::index_sequence<Is...>)                                    \
        {                                                                                       \
            set_generic_return_by<typename traits::return_type>(                                \
                gen,                                                                            \
                func,                                                                           \
                var_type_helper<typename traits::template arg_type<Is>>(                        \
                    var_type_tag<VarType, Is>{},                                                \
                    gen,                                                                        \
                    static_cast<AS_NAMESPACE_QUALIFIER asUINT>(indices[Is])                     \
                )...,                                                                           \
                this_(gen)                                                                      \
            );                                                                                  \
        }(std::make_index_sequence<indices.size()>());                                          \
    }                                                                                           \
    template <typename VarType>                                                                 \
    static void var_type_wrapper_general(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)          \
    {                                                                                           \
        using traits = function_traits<function_type>;                                          \
        static constexpr auto indices = gen_script_arg_idx<traits::arg_count_v>(VarType{});     \
        [gen]<std::size_t... Is>(std::index_sequence<Is...>)                                    \
        {                                                                                       \
            set_generic_return_by<typename traits::return_type>(                                \
                gen,                                                                            \
                func,                                                                           \
                var_type_helper<typename traits::template arg_type<Is>>(                        \
                    var_type_tag<VarType, Is>{},                                                \
                    gen,                                                                        \
                    static_cast<AS_NAMESPACE_QUALIFIER asUINT>(indices[Is])                     \
                )...                                                                            \
            );                                                                                  \
        }(std::make_index_sequence<indices.size()>());                                          \
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
            return &wrapper_thiscall;
        else if constexpr(OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST)
            return &wrapper_objfirst;
        else if constexpr(OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST)
            return &wrapper_objlast;
        else
        {
            static_assert(
                OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL ||
                OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_STDCALL
            );
            return &wrapper_general;
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
            return &var_type_wrapper_thiscall<my_var_type>;
        else if constexpr(OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST)
            return &var_type_wrapper_objfirst<my_var_type>;
        else if constexpr(OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST)
            return &var_type_wrapper_objlast<my_var_type>;
        else
        {
            static_assert(
                OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL ||
                OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_STDCALL
            );
            return &var_type_wrapper_general<my_var_type>;
        }
    }
};

template <
    native_function auto Function,
    AS_NAMESPACE_QUALIFIER asECallConvTypes OriginalConv>
requires(OriginalConv != AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
class generic_wrapper_nontype
{
private:
    using function_type = std::decay_t<decltype(Function)>;
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

    static void wrapper_thiscall_objfirst(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
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

    static void wrapper_thiscall_objlast(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
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
    static void var_type_wrapper_thiscall_objfirst(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
    {
        using traits = function_traits<function_type>;
        static constexpr auto indices = gen_script_arg_idx<traits::arg_count_v - 1>(VarType{});
        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            set_generic_return_by<typename traits::return_type>(
                gen,
                Function,
                get_generic_auxiliary<typename traits::class_type*>(gen),
                this_(gen),
                var_type_helper<typename traits::template arg_type<Is + 1>>( // Plus 1 to skip the "this_(gen)" argument
                    var_type_tag<VarType, Is>{},
                    gen,
                    static_cast<AS_NAMESPACE_QUALIFIER asUINT>(indices[Is])
                )...
            );
        }(std::make_index_sequence<indices.size()>());
    }

    template <typename VarType>
    static void var_type_wrapper_thiscall_objlast(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
    {
        using traits = function_traits<function_type>;
        static constexpr auto indices = gen_script_arg_idx<traits::arg_count_v - 1>(VarType{});
        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            set_generic_return_by<typename traits::return_type>(
                gen,
                Function,
                get_generic_auxiliary<typename traits::class_type*>(gen),
                var_type_helper<typename traits::template arg_type<Is>>(
                    var_type_tag<VarType, Is>{},
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
            return &wrapper_thiscall;
        else if constexpr(OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST)
            return &wrapper_objfirst;
        else if constexpr(OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST)
            return &wrapper_objlast;
        else if constexpr(OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_OBJFIRST)
            return &wrapper_thiscall_objfirst;
        else if constexpr(OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_OBJLAST)
            return &wrapper_thiscall_objlast;
        else
            return &wrapper_general;
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
            return &var_type_wrapper_thiscall<my_var_type>;
        else if constexpr(OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST)
            return &var_type_wrapper_objfirst<my_var_type>;
        else if constexpr(OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST)
            return &var_type_wrapper_objlast<my_var_type>;
        else if constexpr(OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_OBJFIRST)
            return &var_type_wrapper_thiscall_objfirst<my_var_type>;
        else if constexpr(OriginalConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_OBJLAST)
            return &var_type_wrapper_thiscall_objlast<my_var_type>;
        else
            return &var_type_wrapper_general<my_var_type>;
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
        static constexpr auto indices = gen_script_arg_idx<traits::arg_count_v>(VarType{});
        [gen]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            set_generic_return_by<typename traits::return_type>(
                gen,
                Function,
                this_(gen),
                var_type_helper<typename traits::template arg_type<Is>>(
                    var_type_tag<VarType, Is>{},
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

template <
    noncapturing_lambda Lambda,
    AS_NAMESPACE_QUALIFIER asECallConvTypes OriginalCallConv>
requires(OriginalCallConv != AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
consteval auto to_asGENFUNC_t(const Lambda&, call_conv_t<OriginalCallConv>)
    -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
{
    return lambda_to_asGENFUNC_t_impl<Lambda, OriginalCallConv>();
}

template <
    native_function auto Function,
    AS_NAMESPACE_QUALIFIER asECallConvTypes OriginalCallConv>
requires(OriginalCallConv != AS_NAMESPACE_QUALIFIER asCALL_GENERIC)
consteval auto to_asGENFUNC_t(fp_wrapper<Function>, call_conv_t<OriginalCallConv>)
    -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
{
    return fp_to_asGENFUNC_t_impl<Function, OriginalCallConv>();
}

template <
    noncapturing_lambda Lambda,
    AS_NAMESPACE_QUALIFIER asECallConvTypes OriginalCallConv,
    std::size_t... Is>
consteval auto to_asGENFUNC_t(const Lambda&, call_conv_t<OriginalCallConv>, var_type_t<Is...>)
    -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
{
    return lambda_to_asGENFUNC_t_impl<Lambda, OriginalCallConv, Is...>();
}

template <
    native_function auto Function,
    AS_NAMESPACE_QUALIFIER asECallConvTypes OriginalCallConv,
    std::size_t... Is>
consteval auto to_asGENFUNC_t(fp_wrapper<Function>, call_conv_t<OriginalCallConv>, var_type_t<Is...>)
    -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
{
    return fp_to_asGENFUNC_t_impl<Function, OriginalCallConv, Is...>();
}

template <
    native_function auto Function,
    auto Composite>
consteval auto to_asGENFUNC_t(
    fp_wrapper<Function>,
    call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_THISCALL>, // Calling convention parameter reserved for the future
    composite_wrapper_nontype<Composite>
)
    -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
{
    return fp_to_asGENFUNC_t_impl_comp<Function, Composite>();
}

template <
    native_function auto Function,
    auto Composite,
    std::size_t... Is>
consteval auto to_asGENFUNC_t(
    fp_wrapper<Function>,
    call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_THISCALL>, // Calling convention parameter reserved for the future
    composite_wrapper_nontype<Composite>,
    var_type_t<Is...>
)
    -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
{
    return fp_to_asGENFUNC_t_impl_comp<Function, Composite, Is...>();
}

template <
    native_function auto ConstructorFunc,
    typename Class,
    bool Template,
    AS_NAMESPACE_QUALIFIER asECallConvTypes OriginalCallConv>
class generic_wrapper_ctor_func
{
private:
    using native_function_type = std::decay_t<decltype(ConstructorFunc)>;
    using traits = function_traits<decltype(ConstructorFunc)>;
    using args_tuple = typename traits::args_tuple;

    static void wrapper_objfirst(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
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
    }

    static void wrapper_objlast(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
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
    }

public:
    static constexpr auto generate() noexcept
        -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
    {
        if constexpr(OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST)
            return &wrapper_objfirst;
        else // OriginalCallConv == asCALL_CDECL_OBJLAST
            return &wrapper_objlast;
    }
};

template <
    typename ConstructorLambda,
    typename Class,
    bool Template,
    AS_NAMESPACE_QUALIFIER asECallConvTypes OriginalCallConv>
class generic_wrapper_ctor_lambda
{
private:
    using native_function_type = std::decay_t<decltype(+ConstructorLambda{})>;
    using traits = function_traits<native_function_type>;
    using args_tuple = typename traits::args_tuple;

    static void wrapper_objfirst(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
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
    }

    static void wrapper_objlast(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
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
    }

public:
    static constexpr auto generate() noexcept
        -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
    {
        if constexpr(OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST)
            return &wrapper_objfirst;
        else // OriginalCallConv == asCALL_CDECL_OBJLAST
            return &wrapper_objlast;
    }
};

template <
    typename Class,
    bool IsTemplate,
    native_function auto ConstructorFunc,
    AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
requires(
    CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST ||
    CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST
)
constexpr auto constructor_to_asGENFUNC_t(
    fp_wrapper<ConstructorFunc>,
    call_conv_t<CallConv>
)
{
    using gen_t = generic_wrapper_ctor_func<
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
    AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
requires(
    CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST ||
    CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST
)
constexpr auto constructor_to_asGENFUNC_t(
    const ConstructorLambda&,
    call_conv_t<CallConv>
)
{
    using gen_t = generic_wrapper_ctor_lambda<
        ConstructorLambda,
        Class,
        IsTemplate,
        CallConv>;
    return gen_t::generate();
}

template <
    native_function auto ListConstructorFunc,
    typename Class,
    bool IsTemplate,
    AS_NAMESPACE_QUALIFIER asECallConvTypes OriginalCallConv>
class generic_wrapper_list_ctor
{
    // FIXME: Handle IsTemplate

    using traits = function_traits<std::decay_t<decltype(ListConstructorFunc)>>;
    static_assert(traits::arg_count_v == 2);
    static_assert(std::is_void_v<typename traits::return_type>);

    static void wrapper_objfirst(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
    {
        using list_buf_t = typename traits::last_arg_type;
        static_assert(std::is_pointer_v<list_buf_t>);
        ListConstructorFunc(
            get_generic_object<Class*>(gen),
            *(list_buf_t*)gen->GetAddressOfArg(0)
        );
    }

    static void wrapper_objlast(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
    {
        using list_buf_t = typename traits::first_arg_type;
        static_assert(std::is_pointer_v<list_buf_t>);
        ListConstructor(
            *(list_buf_t*)gen->GetAddressOfArg(0),
            get_generic_object<Class*>(gen)
        );
    }

public:
    static constexpr auto generate() noexcept
        -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
    {
        if constexpr(OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST)
            return &wrapper_objfirst;
        else // OriginalCallConv == asCALL_CDECL_OBJLAST
            return &wrapper_objlast;
    }
};

template <
    typename Class,
    bool IsTemplate,
    native_function auto ListConstructorFunc,
    AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
requires(
    CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST ||
    CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST
)
constexpr auto list_constructor_to_asGENFUNC_t(
    fp_wrapper<ListConstructorFunc>,
    call_conv_t<CallConv>
)
{
    using gen_t = generic_wrapper_list_ctor<
        ListConstructorFunc,
        Class,
        IsTemplate,
        CallConv>;
    return gen_t::generate();
}

// Generic wrapper for the factory with an auxiliary object
// (Ordinary factories can be treated as global functions)
// Note: although the global C++ functions are registered as CDECL_OBJFIRST/LAST,
// the auxiliary object is still retrieved through GetAuxiliary() instead of GetObject()
template <
    native_function auto FactoryFunc,
    bool Template,
    AS_NAMESPACE_QUALIFIER asECallConvTypes OriginalCallConv>
class generic_wrapper_factory_aux
{
    using traits = function_traits<decltype(FactoryFunc)>;
    using args_tuple = typename traits::args_tuple;
    // For THISCALL_ASGLOBAL, we'll directly use traits::class_type
    using auxiliary_type = std::conditional_t<
        OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST,
        typename traits::first_arg_type,
        typename traits::last_arg_type>;

    // For templated classes
    static void* invoke_factory(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
        requires(Template)
    {
        // Argument count except the typeinfo and auxiliary object
        constexpr std::size_t user_arg_v =
            OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL ?
                traits::arg_count_v - 1 :
                traits::arg_count_v - 2;

        return [gen]<std::size_t... Is>(std::index_sequence<Is...>) -> void*
        {
            if constexpr(OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL)
            {
                return std::invoke(
                    FactoryFunc,
                    get_generic_auxiliary<typename traits::class_type>(gen),
                    *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0),
                    get_generic_arg<std::tuple_element_t<Is + 1, args_tuple>>(gen, (AS_NAMESPACE_QUALIFIER asUINT)Is + 1)...
                );
            }
            else if constexpr(OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST)
            {
                return std::invoke(
                    FactoryFunc,
                    get_generic_auxiliary<auxiliary_type>(gen),
                    *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0),
                    get_generic_arg<std::tuple_element_t<Is + 2, args_tuple>>(gen, (AS_NAMESPACE_QUALIFIER asUINT)Is + 1)...
                );
            }
            else // OriginalCallConv == asCALL_CDECL_OBJLAST
            {
                return std::invoke(
                    FactoryFunc,
                    *(AS_NAMESPACE_QUALIFIER asITypeInfo**)gen->GetAddressOfArg(0),
                    get_generic_arg<std::tuple_element_t<Is + 1, args_tuple>>(gen, (AS_NAMESPACE_QUALIFIER asUINT)Is + 1)...,
                    get_generic_auxiliary<auxiliary_type>(gen)
                );
            }
        }(std::make_index_sequence<user_arg_v>());
    }

    // For non-templated classes
    static void* invoke_factory(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
        requires(!Template)
    {
        // Argument count except the auxiliary object
        constexpr std::size_t user_arg_v =
            OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL ?
                traits::arg_count_v :
                traits::arg_count_v - 1;

        return [gen]<std::size_t... Is>(std::index_sequence<Is...>) -> void*
        {
            if constexpr(OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL)
            {
                return std::invoke(
                    FactoryFunc,
                    get_generic_auxiliary<typename traits::class_type>(gen),
                    get_generic_arg<std::tuple_element_t<Is, args_tuple>>(gen, (AS_NAMESPACE_QUALIFIER asUINT)Is)...
                );
            }
            else if constexpr(OriginalCallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST)
            {
                return std::invoke(
                    FactoryFunc,
                    get_generic_auxiliary<auxiliary_type>(gen),
                    // Plus 1 to skip the auxiliary object at the first position
                    get_generic_arg<std::tuple_element_t<Is + 1, args_tuple>>(gen, (AS_NAMESPACE_QUALIFIER asUINT)Is)...
                );
            }
            else // OriginalCallConv == asCALL_CDECL_OBJLAST
            {
                return std::invoke(
                    FactoryFunc,
                    get_generic_arg<std::tuple_element_t<Is, args_tuple>>(gen, (AS_NAMESPACE_QUALIFIER asUINT)Is)...,
                    get_generic_auxiliary<auxiliary_type>(gen)
                );
            }
        }(std::make_index_sequence<user_arg_v>());
    }

    static void wrapper_impl(AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen)
    {
        void* ptr = invoke_factory(gen);
        gen->SetReturnAddress(ptr);
    }

public:
    static constexpr auto generate() noexcept
        -> AS_NAMESPACE_QUALIFIER asGENFUNC_t
    {
        return &wrapper_impl;
    }
};

template <
    bool IsTemplate,
    auto AuxFactoryFunc,
    AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
requires(
    CallConv == AS_NAMESPACE_QUALIFIER asCALL_THISCALL_ASGLOBAL ||
    CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJFIRST ||
    CallConv == AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST
)
constexpr auto auxiliary_factory_to_asGENFUNC_t(
    fp_wrapper<AuxFactoryFunc>,
    call_conv_t<CallConv>
)
{
    using gen_t = generic_wrapper_factory_aux<
        AuxFactoryFunc,
        IsTemplate,
        CallConv>;
    return gen_t::generate();
}
} // namespace asbind20::detail

#endif
