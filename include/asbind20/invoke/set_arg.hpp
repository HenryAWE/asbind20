#ifndef ASBIND20_INVOKE_SET_ARG_HPP
#define ASBIND20_INVOKE_SET_ARG_HPP

#include "../fwd.hpp"
#include "../type_traits.hpp"
#include "../util/unreachable.hpp"

namespace asbind20
{
#if defined(__GNUC__) || defined(__clang__)
#    pragma GCC diagnostic push
// Some wrappers still need C-style cast to convert any address into void* pointer
#    pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

template <typename T>
int set_script_arg(
    context_pointer ctx,
    arg_index_type idx,
    std::reference_wrapper<T> ref
)
{
    return ctx->SetArgAddress(idx, (void*)std::addressof(ref.get()));
}

template <std::integral T>
int set_script_arg(
    context_pointer ctx,
    arg_index_type idx,
    const T& val
)
{
    constexpr std::size_t int_size = sizeof(std::decay_t<T>);

    if constexpr(int_size == sizeof(AS_NAMESPACE_QUALIFIER asBYTE))
        return ctx->SetArgByte(idx, val);
    else if constexpr(int_size == sizeof(AS_NAMESPACE_QUALIFIER asWORD))
        return ctx->SetArgWord(idx, val);
    else if constexpr(int_size == sizeof(AS_NAMESPACE_QUALIFIER asDWORD))
        return ctx->SetArgDWord(idx, val);
    else if constexpr(int_size == sizeof(AS_NAMESPACE_QUALIFIER asQWORD))
        return ctx->SetArgQWord(idx, val);
    else
    {
        // Built-in (u)int128
        return ctx->SetArgObject(idx, (void*)std::addressof(val));
    }
}

template <typename Enum>
requires std::is_enum_v<Enum>
int set_script_arg(
    context_pointer ctx,
    arg_index_type idx,
    Enum val
)
{
    using type = std::remove_cv_t<Enum>;

    constexpr bool is_customized = requires() {
        { type_traits<type>::set_arg(*ctx, idx, val) } -> std::same_as<int>;
    };

    if constexpr(is_customized)
    {
        return type_traits<type>::set_arg(*ctx, idx, val);
    }
    else
    {
        // Keep casting to int even in new AS version,
        // otherwise AS will report error.
        return set_script_arg(ctx, idx, static_cast<int>(val));
    }
}

template <std::floating_point T>
int set_script_arg(
    context_pointer ctx,
    arg_index_type idx,
    T val
)
{
    using type = std::remove_cv_t<T>;

    if constexpr(std::same_as<type, float>)
        return ctx->SetArgFloat(idx, val);
    else if constexpr(std::same_as<type, double>)
        return ctx->SetArgDouble(idx, val);
    else
        static_assert(!sizeof(T), "Invalid floating point");

    // Suppress warning
    util::unreachable();
}

inline int set_script_arg(
    context_pointer ctx,
    arg_index_type idx,
    void* obj
)
{
    return ctx->SetArgAddress(idx, obj);
}

inline int set_script_arg(
    context_pointer ctx,
    arg_index_type idx,
    const void* obj
)
{
    return ctx->SetArgAddress(idx, const_cast<void*>(obj));
}

inline int set_script_arg(
    context_pointer ctx,
    arg_index_type idx,
    object_pointer obj
)
{
    return ctx->SetArgObject(idx, obj);
}

inline int set_script_arg(
    context_pointer ctx,
    arg_index_type idx,
    const_object_pointer obj
)
{
    return ctx->SetArgObject(idx, const_cast<object_pointer>(obj));
}

template <typename Class>
requires std::is_class_v<std::remove_cvref_t<Class>>
int set_script_arg(
    context_pointer ctx,
    arg_index_type idx,
    Class&& obj
)
{
    using type = std::remove_cvref_t<Class>;

    constexpr bool is_customized = requires() {
        { type_traits<type>::set_script_arg(ctx, idx, obj) } -> std::same_as<int>;
    };

    if constexpr(is_customized)
    {
        return type_traits<type>::set_script_arg(ctx, idx, obj);
    }
    else
    {
        return ctx->SetArgObject(idx, (void*)std::addressof(obj));
    }
}

#if defined(__GNUC__) || defined(__clang__)
#    pragma GCC diagnostic pop
#endif

/**
 * @brief Apply a tuple to script context as arguments
 *
 * @param ctx Script context. The script function must be prepared.
 * @param tp Tuple of arguments
 */
template <typename Tuple>
void apply_script_args(context_pointer ctx, Tuple&& tp)
{
    [&]<arg_index_type... Idx>(std::integer_sequence<arg_index_type, Idx...>)
    {
        (set_script_arg(ctx, Idx, std::get<Idx>(tp)), ...);
    }(std::make_integer_sequence<arg_index_type, std::tuple_size_v<Tuple>>());
}

inline int set_script_object(
    context_pointer ctx, const void* obj
)
{
    return ctx->SetObject(const_cast<void*>(obj));
}

template <script_object_handle Object>
int set_script_object(
    context_pointer ctx, Object&& obj
)
{
    const void* ptr = const_object_pointer(obj);
    return set_script_object(ctx, ptr);
}
} // namespace asbind20

#endif
