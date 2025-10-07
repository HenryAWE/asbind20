/**
 * @file utility.hpp
 * @author HenryAWE
 * @brief Utilities for AngelScript and binding generator
 */

#ifndef ASBIND20_UTILITY_HPP
#define ASBIND20_UTILITY_HPP

#pragma once

#include <cassert>
#include <cstddef>
#include <cstring>
#include <string>
#include <utility>
#include <bit>
#include <mutex> // IWYU pragma: export `std::lock_guard`
#include <compare>
#include <functional>
#include <type_traits>
#include <concepts>
#include "detail/config.hpp" // IWYU pragma: export configs
#include "detail/include_as.hpp"

namespace asbind20
{
template <typename FirstPolicy = void, typename... Policies>
struct use_policy_t
{
    using first_policy = FirstPolicy;
    using policies_tuple = std::tuple<FirstPolicy, Policies...>;
};

template <typename FirstPolicy = void, typename... Policies>
constexpr inline use_policy_t<FirstPolicy, Policies...> use_policy{};

template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
struct call_conv_t
{};

/**
 * @brief Helper for specifying calling convention
 */
template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
constexpr inline call_conv_t<CallConv> call_conv;

constexpr inline call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_GENERIC> generic_call_conv{};

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
struct fp_wrapper
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
constexpr inline fp_wrapper<Function> fp{};

struct this_type_t
{};

/**
 * @brief Tag indicating current type. Its exact meaning depends on context.
 */
inline constexpr this_type_t this_type{};

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

/**
 * @brief Get offset from a member pointer
 *
 * @note This function is implemented by undefined behavior but is expected to work on most platforms
 */
template <typename T, typename Class>
std::size_t member_offset(T Class::* mp) noexcept
{
    Class* p = nullptr;
    return std::size_t(std::addressof(p->*mp)) - std::size_t(p);
}

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
    template <typename... Args>
    struct overload_cast_impl
    {
        template <typename Return>
        constexpr auto operator()(Return (*pfn)(Args...)) const noexcept
            -> decltype(pfn)
        {
            return pfn;
        }

        template <typename Return, typename Class>
        constexpr auto operator()(Return (Class::*mp)(Args...), std::false_type = {}) const noexcept
            -> decltype(mp)
        {
            return mp;
        }

        template <typename Return, typename Class>
        constexpr auto operator()(Return (Class::*mp)(Args...) const, std::true_type) const noexcept
            -> decltype(mp)
        {
            return mp;
        }
    };
} // namespace detail

/**
 * @brief Tag for indicating const member
 */
constexpr inline std::true_type const_;

/**
 * @brief Helper for choosing overloaded function
 *
 * @tparam Args Parameters of the desired function
 */
template <typename... Args>
constexpr inline detail::overload_cast_impl<Args...> overload_cast{};

template <int TypeId>
requires(!(TypeId & ~(AS_NAMESPACE_QUALIFIER asTYPEID_MASK_SEQNBR)))
struct primitive_type_of;

#define ASBIND20_UTILITY_DEFINE_PRIMITIVE_TYPE_OF(as_type_id, cpp_type, script_decl) \
    template <>                                                                      \
    struct primitive_type_of<AS_NAMESPACE_QUALIFIER as_type_id>                      \
    {                                                                                \
        using type = cpp_type;                                                       \
        static constexpr char decl[] = script_decl;                                  \
    };

ASBIND20_UTILITY_DEFINE_PRIMITIVE_TYPE_OF(asTYPEID_VOID, void, "void");
ASBIND20_UTILITY_DEFINE_PRIMITIVE_TYPE_OF(asTYPEID_BOOL, bool, "bool");
ASBIND20_UTILITY_DEFINE_PRIMITIVE_TYPE_OF(asTYPEID_INT8, std::int8_t, "int8");
ASBIND20_UTILITY_DEFINE_PRIMITIVE_TYPE_OF(asTYPEID_INT16, std::int16_t, "int16");
ASBIND20_UTILITY_DEFINE_PRIMITIVE_TYPE_OF(asTYPEID_INT32, std::int32_t, "int32");
ASBIND20_UTILITY_DEFINE_PRIMITIVE_TYPE_OF(asTYPEID_INT64, std::int64_t, "int64");
ASBIND20_UTILITY_DEFINE_PRIMITIVE_TYPE_OF(asTYPEID_UINT8, std::uint8_t, "uint8");
ASBIND20_UTILITY_DEFINE_PRIMITIVE_TYPE_OF(asTYPEID_UINT16, std::uint16_t, "uint16");
ASBIND20_UTILITY_DEFINE_PRIMITIVE_TYPE_OF(asTYPEID_UINT32, std::uint32_t, "uint");
ASBIND20_UTILITY_DEFINE_PRIMITIVE_TYPE_OF(asTYPEID_UINT64, std::uint64_t, "uint64");
ASBIND20_UTILITY_DEFINE_PRIMITIVE_TYPE_OF(asTYPEID_FLOAT, float, "float");
ASBIND20_UTILITY_DEFINE_PRIMITIVE_TYPE_OF(asTYPEID_DOUBLE, double, "double");

#undef ASBIND20_UTILITY_DEFINE_PRIMITIVE_TYPE_OF

template <int TypeId>
using primitive_type_of_t = typename primitive_type_of<TypeId>::type;

/**
 * @brief Check if a type id refers to void
 */
[[nodiscard]]
constexpr bool is_void_type(int type_id) noexcept
{
    return type_id == (AS_NAMESPACE_QUALIFIER asTYPEID_VOID);
}

/**
 * @brief Check if a type id refers to a primitive type
 */
[[nodiscard]]
constexpr bool is_primitive_type(int type_id) noexcept
{
    return !(type_id & ~(AS_NAMESPACE_QUALIFIER asTYPEID_MASK_SEQNBR));
}

/**
 * @brief Check if a type id refers to an enum type
 */
[[nodiscard]]
constexpr bool is_enum_type(int type_id) noexcept
{
    return is_primitive_type(type_id) &&
           type_id > AS_NAMESPACE_QUALIFIER asTYPEID_DOUBLE;
}

[[nodiscard]]
constexpr bool is_floating_point(int type_id) noexcept
{
    return type_id == AS_NAMESPACE_QUALIFIER asTYPEID_FLOAT ||
           type_id == AS_NAMESPACE_QUALIFIER asTYPEID_DOUBLE;
}

[[nodiscard]]
constexpr bool is_integral(int type_id) noexcept
{
    return (type_id >= AS_NAMESPACE_QUALIFIER asTYPEID_BOOL &&
            type_id <= AS_NAMESPACE_QUALIFIER asTYPEID_UINT64) ||
           is_enum_type(type_id);
}

[[nodiscard]]
constexpr bool is_bool_type(int type_id) noexcept
{
    return type_id == AS_NAMESPACE_QUALIFIER asTYPEID_BOOL;
}

[[nodiscard]]
constexpr bool is_unsigned(int type_id) noexcept
{
    return type_id == AS_NAMESPACE_QUALIFIER asTYPEID_BOOL ||
           type_id == AS_NAMESPACE_QUALIFIER asTYPEID_UINT8 ||
           type_id == AS_NAMESPACE_QUALIFIER asTYPEID_UINT16 ||
           type_id == AS_NAMESPACE_QUALIFIER asTYPEID_UINT32 ||
           type_id == AS_NAMESPACE_QUALIFIER asTYPEID_UINT64;
}

[[nodiscard]]
constexpr bool is_signed(int type_id) noexcept
{
    return type_id == AS_NAMESPACE_QUALIFIER asTYPEID_INT8 ||
           type_id == AS_NAMESPACE_QUALIFIER asTYPEID_INT16 ||
           type_id == AS_NAMESPACE_QUALIFIER asTYPEID_INT32 ||
           type_id == AS_NAMESPACE_QUALIFIER asTYPEID_INT64;
}

/**
 * @brief Check if a type id refers to an object handle
 */
[[nodiscard]]
constexpr bool is_objhandle(int type_id) noexcept
{
    return type_id & (AS_NAMESPACE_QUALIFIER asTYPEID_OBJHANDLE);
}

/**
 * @brief Get the script string type ID
 *
 * @param engine Script engine
 */
[[nodiscard]]
inline int get_script_string_type(
    const AS_NAMESPACE_QUALIFIER asIScriptEngine* engine
)
{
#if ANGELSCRIPT_VERSION >= 23800
    int string_t_id = engine->GetStringFactory();
#else
    int string_t_id = engine->GetStringFactoryReturnTypeId();
#endif

    return string_t_id;
}

/**
 * @brief Check if a type id refers to the script string
 */
[[nodiscard]]
inline bool is_script_string(
    const AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, int type_id
)
{
    return get_script_string_type(engine) == type_id;
}

/**
 * @brief Check if a type requires GC
 *
 * This can be used for template callback.
 *
 * @param ti Type information. Null pointer is allowed for indicating primitive type,
 *           so it's safe to call this function by `type_requires_gc(ti->GetSubType())`.
 */
[[nodiscard]]
inline bool type_requires_gc(const AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
{
    if(!ti) [[unlikely]]
        return false;

    auto flags = ti->GetFlags();

    if(flags & AS_NAMESPACE_QUALIFIER asOBJ_REF)
    {
        if(flags & AS_NAMESPACE_QUALIFIER asOBJ_NOCOUNT)
            return false;
        else
            return true;
    }
    else if((flags & AS_NAMESPACE_QUALIFIER asOBJ_VALUE) &&
            (flags & AS_NAMESPACE_QUALIFIER asOBJ_GC))
    {
        return true;
    }

    return false;
}

/**
 * @brief Get the size of a script type
 *
 * @param engine Script engine
 * @param type_id AngelScript type id
 */
inline auto sizeof_script_type(
    const AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, int type_id
)
    -> AS_NAMESPACE_QUALIFIER asUINT
{
    assert(engine != nullptr);

    if(is_primitive_type(type_id))
    {
        switch(type_id)
        {
        case AS_NAMESPACE_QUALIFIER asTYPEID_VOID:
            return 0;

        case AS_NAMESPACE_QUALIFIER asTYPEID_BOOL:
        case AS_NAMESPACE_QUALIFIER asTYPEID_INT8:
        case AS_NAMESPACE_QUALIFIER asTYPEID_UINT8:
            return sizeof(std::int8_t);

        case AS_NAMESPACE_QUALIFIER asTYPEID_INT16:
        case AS_NAMESPACE_QUALIFIER asTYPEID_UINT16:
            return sizeof(std::int16_t);

        default: // enum
        case AS_NAMESPACE_QUALIFIER asTYPEID_INT32:
        case AS_NAMESPACE_QUALIFIER asTYPEID_UINT32:
            return sizeof(std::int32_t);

        case AS_NAMESPACE_QUALIFIER asTYPEID_INT64:
        case AS_NAMESPACE_QUALIFIER asTYPEID_UINT64:
            return sizeof(std::int64_t);

        case AS_NAMESPACE_QUALIFIER asTYPEID_FLOAT:
            return sizeof(float);
        case AS_NAMESPACE_QUALIFIER asTYPEID_DOUBLE:
            return sizeof(double);
        }
    }

    auto* ti = engine->GetTypeInfoById(type_id);
    if(!ti)
        return 0;

    return ti->GetSize();
}

/**
 * @brief Copy a single primitive value
 *
 * @param dst Destination pointer
 * @param src Source pointer
 * @param type_id AngelScript type id
 * @return Bytes copied
 *
 * @warning Please make sure the destination has enough space for the value
 */
inline std::size_t copy_primitive_value(void* dst, const void* src, int type_id)
{
    assert(is_primitive_type(type_id));

    switch(type_id)
    {
    [[unlikely]] case AS_NAMESPACE_QUALIFIER asTYPEID_VOID:
        return 0;

    case AS_NAMESPACE_QUALIFIER asTYPEID_BOOL:
    case AS_NAMESPACE_QUALIFIER asTYPEID_INT8:
    case AS_NAMESPACE_QUALIFIER asTYPEID_UINT8:
        *(std::uint8_t*)dst = *(std::uint8_t*)src;
        return sizeof(std::uint8_t);

    case AS_NAMESPACE_QUALIFIER asTYPEID_INT16:
    case AS_NAMESPACE_QUALIFIER asTYPEID_UINT16:
        *(std::uint16_t*)dst = *(std::uint16_t*)src;
        return sizeof(std::uint16_t);

    default: // enums
    case AS_NAMESPACE_QUALIFIER asTYPEID_FLOAT:
    case AS_NAMESPACE_QUALIFIER asTYPEID_INT32:
    case AS_NAMESPACE_QUALIFIER asTYPEID_UINT32:
        *(std::uint32_t*)dst = *(std::uint32_t*)src;
        return sizeof(std::uint32_t);

    case AS_NAMESPACE_QUALIFIER asTYPEID_DOUBLE:
    case AS_NAMESPACE_QUALIFIER asTYPEID_INT64:
    case AS_NAMESPACE_QUALIFIER asTYPEID_UINT64:
        *(std::uint64_t*)dst = *(std::uint64_t*)src;
        return sizeof(std::uint64_t);
    }
}

template <typename... Ts>
class overloaded : public Ts...
{
public:
    using Ts::operator()...;
};

template <typename... Ts>
overloaded(Ts&&...) -> overloaded<Ts...>;

template <typename T>
concept void_ptr = std::is_pointer_v<std::decay_t<T>> &&
                   std::is_void_v<std::remove_pointer_t<std::decay_t<T>>>;

/**
 * @brief Dispatches pointer of primitive values to corresponding type. Similar to the `std::visit`.
 *
 * @warning This function disallows void type (`asTYPEID_VOID`)
 *
 * @param vis Callable object that can accept all kinds of pointers to primitive types
 * @param type_id AngelScript TypeId
 * @param args Pointers to primitive values
 */
template <typename Visitor, void_ptr... VoidPtrs>
requires(sizeof...(VoidPtrs) > 0)
decltype(auto) visit_primitive_type(Visitor&& vis, int type_id, VoidPtrs... args)
{
    assert(is_primitive_type(type_id) && "Must be a primitive type");
    assert(!is_void_type(type_id) && "Must not be void");

    auto wrapper = [&]<typename T>(std::in_place_type_t<T>) -> decltype(auto)
    {
        return std::invoke(
            std::forward<Visitor>(vis),
            ((typename std::pointer_traits<VoidPtrs>::template rebind<T>)args)...
        );
    };

#define ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE(as_type_id) \
case AS_NAMESPACE_QUALIFIER as_type_id:                        \
    return wrapper(std::in_place_type<primitive_type_of_t<AS_NAMESPACE_QUALIFIER as_type_id>>)

    switch(type_id)
    {
        ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE(asTYPEID_BOOL);
        ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE(asTYPEID_INT8);
        ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE(asTYPEID_INT16);
    default: /* enums */
        ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE(asTYPEID_INT32);
        ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE(asTYPEID_INT64);
        ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE(asTYPEID_UINT8);
        ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE(asTYPEID_UINT16);
        ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE(asTYPEID_UINT32);
        ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE(asTYPEID_UINT64);
        ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE(asTYPEID_FLOAT);
        ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE(asTYPEID_DOUBLE);
    }

#undef ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE
}

/**
 * @brief Dispatches pointer of values to corresponding type. Similar to the `std::visit`.
 *
 * @warning This function disallows void type (`asTYPEID_VOID`)
 * @note The object handle will be converted to `void**`, while script class and registered type will retain as `void*`
 *
 * @param vis Callable object that can accept all kind of pointer
 * @param type_id AngelScript TypeId
 * @param args Pointers to values
 */
template <typename Visitor, void_ptr... VoidPtrs>
requires(sizeof...(VoidPtrs) > 0)
decltype(auto) visit_script_type(Visitor&& vis, int type_id, VoidPtrs... args)
{
    if(is_primitive_type(type_id))
        return visit_primitive_type(std::forward<Visitor>(vis), type_id, args...);
    else if(type_id & (AS_NAMESPACE_QUALIFIER asTYPEID_OBJHANDLE))
    {
        return std::invoke(
            std::forward<Visitor>(vis),
            ((typename std::pointer_traits<VoidPtrs>::template rebind<void*>)args)...
        );
    }
    else
    {
        return std::invoke(
            std::forward<Visitor>(vis),
            args...
        );
    }
}

/**
 * @brief Convert primitive type ID to corresponding `std::in_place_type<T>`
 *
 * @tparam Visitor Callable that accepts the `std::in_place_type<T>`
 */
template <typename Visitor>
decltype(auto) visit_primitive_type_id(Visitor&& vis, int type_id)
{
    assert(is_primitive_type(type_id));

#define ASBIND20_UTILITY_VISIT_SCRIPT_TYPE_ID_CASE(as_type_id)                     \
case AS_NAMESPACE_QUALIFIER as_type_id:                                            \
    return std::invoke(                                                            \
        std::forward<Visitor>(vis),                                                \
        std::in_place_type<primitive_type_of_t<AS_NAMESPACE_QUALIFIER as_type_id>> \
    )

    switch(type_id)
    {
        ASBIND20_UTILITY_VISIT_SCRIPT_TYPE_ID_CASE(asTYPEID_VOID);
        ASBIND20_UTILITY_VISIT_SCRIPT_TYPE_ID_CASE(asTYPEID_BOOL);
        ASBIND20_UTILITY_VISIT_SCRIPT_TYPE_ID_CASE(asTYPEID_INT8);
        ASBIND20_UTILITY_VISIT_SCRIPT_TYPE_ID_CASE(asTYPEID_INT16);
    default: // enums
        ASBIND20_UTILITY_VISIT_SCRIPT_TYPE_ID_CASE(asTYPEID_INT32);
        ASBIND20_UTILITY_VISIT_SCRIPT_TYPE_ID_CASE(asTYPEID_INT64);
        ASBIND20_UTILITY_VISIT_SCRIPT_TYPE_ID_CASE(asTYPEID_UINT8);
        ASBIND20_UTILITY_VISIT_SCRIPT_TYPE_ID_CASE(asTYPEID_UINT16);
        ASBIND20_UTILITY_VISIT_SCRIPT_TYPE_ID_CASE(asTYPEID_UINT32);
        ASBIND20_UTILITY_VISIT_SCRIPT_TYPE_ID_CASE(asTYPEID_UINT64);
        ASBIND20_UTILITY_VISIT_SCRIPT_TYPE_ID_CASE(asTYPEID_FLOAT);
        ASBIND20_UTILITY_VISIT_SCRIPT_TYPE_ID_CASE(asTYPEID_DOUBLE);
    }

#undef ASBIND20_UTILITY_VISIT_SCRIPT_TYPE_ID_CASE
}

namespace detail
{
    class as_exclusive_lock_t
    {
    public:
        static void lock()
        {
            AS_NAMESPACE_QUALIFIER asAcquireExclusiveLock();
        }

        static void unlock()
        {
            AS_NAMESPACE_QUALIFIER asReleaseExclusiveLock();
        }
    };

    class as_shared_lock_t
    {
    public:
        static void lock()
        {
            AS_NAMESPACE_QUALIFIER asAcquireSharedLock();
        }

        static void unlock()
        {
            AS_NAMESPACE_QUALIFIER asReleaseSharedLock();
        }
    };
} // namespace detail

/**
 * @brief Wrapper for `asAcquireExclusiveLock()` and `asReleaseExclusiveLock()`
 */
inline constexpr detail::as_exclusive_lock_t as_exclusive_lock = {};


/**
 * @brief Wrapper for `asAcquireSharedLock()` and `asReleaseSharedLock()`
 */
inline constexpr detail::as_shared_lock_t as_shared_lock = {};

namespace detail
{
    constexpr void concat_impl(std::string& out, const std::string& str)
    {
        out += str;
    }

    template <std::convertible_to<std::string_view> StringView>
    constexpr void concat_impl(std::string& out, StringView sv)
    {
        out.append(sv);
    }

    constexpr void concat_impl(std::string& out, const char* cstr)
    {
        out.append(cstr);
    }

    template <std::size_t N>
    constexpr void concat_impl(std::string& out, const char (&str)[N])
    {
        if(str[N - 1] == '\0') [[likely]]
            out.append(str, N - 1);
        else
            out.append(str, N);
    }

    constexpr void concat_impl(std::string& out, char ch)
    {
        out.append(1, ch);
    }

    constexpr std::size_t concat_size(const std::string& str)
    {
        return str.size();
    }

    template <std::convertible_to<std::string_view> StringView>
    constexpr std::size_t concat_size(StringView sv)
    {
        return std::string_view(sv).size();
    }

    constexpr std::size_t concat_size(const char* cstr)
    {
        return std::char_traits<char>::length(cstr);
    }

    template <std::size_t N>
    constexpr std::size_t concat_size(const char (&str)[N])
    {
        if(str[N - 1] == '\0') [[likely]]
            return N - 1;
        else
            return N;
    }

    constexpr std::size_t concat_size(char ch)
    {
        (void)ch;
        return 1;
    }

    template <typename T>
    concept concat_accepted =
        std::same_as<std::remove_cvref_t<T>, std::string> ||
        std::convertible_to<std::decay_t<T>, const char*> ||
        std::convertible_to<T, std::string_view> ||
        std::same_as<std::remove_cvref_t<T>, char>;
} // namespace detail

/**
 * @brief Concatenate strings
 *
 * @param out Output string, must be empty
 * @param args String-like inputs
 * @return std::string& Reference to the output
 */
template <detail::concat_accepted... Args>
constexpr std::string& string_concat_inplace(std::string& out, Args&&... args)
{
    if constexpr(sizeof...(Args) > 0)
    {
        std::size_t sz = out.size() + (detail::concat_size(args) + ...);
        out.reserve(sz);

        (detail::concat_impl(out, std::forward<Args>(args)), ...);
    }

    return out;
}

/**
 * @brief Concatenate strings
 *
 * @param args String-like inputs
 * @return std::string Result
 */
template <detail::concat_accepted... Args>
constexpr std::string string_concat(Args&&... args)
{
    std::string out;
    string_concat_inplace(out, std::forward<Args>(args)...);
    return out;
}

/**
 * @brief Convert context state enum to string
 *
 * @param state Context state
 * @return String representation of the state.
 *         If the state value is invalid, the result will be `"asEContextState({state})"`,
 *         e.g. `"asEContextState(-1)"`.
 */
inline std::string to_string(AS_NAMESPACE_QUALIFIER asEContextState state)
{
    switch(state)
    {
    case AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED:
        return "asEXECUTION_FINISHED";
    case AS_NAMESPACE_QUALIFIER asEXECUTION_SUSPENDED:
        return "asEXECUTION_SUSPENDED";
    case AS_NAMESPACE_QUALIFIER asEXECUTION_ABORTED:
        return "asEXECUTION_ABORTED";
    case AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION:
        return "asEXECUTION_EXCEPTION";
    case AS_NAMESPACE_QUALIFIER asEXECUTION_PREPARED:
        return "asEXECUTION_PREPARED";
    case AS_NAMESPACE_QUALIFIER asEXECUTION_UNINITIALIZED:
        return "asEXECUTION_UNINITIALIZED";
    case AS_NAMESPACE_QUALIFIER asEXECUTION_ACTIVE:
        return "asEXECUTION_ACTIVE";
    case AS_NAMESPACE_QUALIFIER asEXECUTION_ERROR:
        return "asEXECUTION_ERROR";
    case AS_NAMESPACE_QUALIFIER asEXECUTION_DESERIALIZATION:
        return "asEXECUTION_DESERIALIZATION";

    [[unlikely]] default:
        return string_concat(
            "asEContextState(",
            std::to_string(static_cast<int>(state)),
            ')'
        );
    }
}

/**
 * @brief Convert return code to string
 *
 * @param ret Return code
 * @return String representation of the return code.
 *         If the value is invalid, the result will be `"asERetCodes({ret})"`,
 *         e.g. `"asERetCodes(1)"`.
 */
inline std::string to_string(AS_NAMESPACE_QUALIFIER asERetCodes ret)
{
    switch(ret)
    {
    case AS_NAMESPACE_QUALIFIER asSUCCESS:
        return "asSUCCESS";
    case AS_NAMESPACE_QUALIFIER asERROR:
        return "asERROR";
    case AS_NAMESPACE_QUALIFIER asCONTEXT_ACTIVE:
        return "asCONTEXT_ACTIVE";
    case AS_NAMESPACE_QUALIFIER asCONTEXT_NOT_FINISHED:
        return "asCONTEXT_NOT_FINISHED";
    case AS_NAMESPACE_QUALIFIER asCONTEXT_NOT_PREPARED:
        return "asCONTEXT_NOT_PREPARED";
    case AS_NAMESPACE_QUALIFIER asINVALID_ARG:
        return "asINVALID_ARG";
    case AS_NAMESPACE_QUALIFIER asNO_FUNCTION:
        return "asNO_FUNCTION";
    case AS_NAMESPACE_QUALIFIER asNOT_SUPPORTED:
        return "asNOT_SUPPORTED";
    case AS_NAMESPACE_QUALIFIER asINVALID_NAME:
        return "asINVALID_NAME";
    case AS_NAMESPACE_QUALIFIER asNAME_TAKEN:
        return "asNAME_TAKEN";
    case AS_NAMESPACE_QUALIFIER asINVALID_DECLARATION:
        return "asINVALID_DECLARATION";
    case AS_NAMESPACE_QUALIFIER asINVALID_OBJECT:
        return "asINVALID_OBJECT";
    case AS_NAMESPACE_QUALIFIER asINVALID_TYPE:
        return "asINVALID_TYPE";
    case AS_NAMESPACE_QUALIFIER asALREADY_REGISTERED:
        return "asALREADY_REGISTERED";
    case AS_NAMESPACE_QUALIFIER asMULTIPLE_FUNCTIONS:
        return "asMULTIPLE_FUNCTIONS";
    case AS_NAMESPACE_QUALIFIER asNO_MODULE:
        return "asNO_MODULE";
    case AS_NAMESPACE_QUALIFIER asNO_GLOBAL_VAR:
        return "asNO_GLOBAL_VAR";
    case AS_NAMESPACE_QUALIFIER asINVALID_CONFIGURATION:
        return "asINVALID_CONFIGURATION";
    case AS_NAMESPACE_QUALIFIER asINVALID_INTERFACE:
        return "asINVALID_INTERFACE";
    case AS_NAMESPACE_QUALIFIER asCANT_BIND_ALL_FUNCTIONS:
        return "asCANT_BIND_ALL_FUNCTIONS";
    case AS_NAMESPACE_QUALIFIER asLOWER_ARRAY_DIMENSION_NOT_REGISTERED:
        return "asLOWER_ARRAY_DIMENSION_NOT_REGISTERED";
    case AS_NAMESPACE_QUALIFIER asWRONG_CONFIG_GROUP:
        return "asWRONG_CONFIG_GROUP";
    case AS_NAMESPACE_QUALIFIER asCONFIG_GROUP_IS_IN_USE:
        return "asCONFIG_GROUP_IS_IN_USE";
    case AS_NAMESPACE_QUALIFIER asILLEGAL_BEHAVIOUR_FOR_TYPE:
        return "asILLEGAL_BEHAVIOUR_FOR_TYPE";
    case AS_NAMESPACE_QUALIFIER asWRONG_CALLING_CONV:
        return "asWRONG_CALLING_CONV";
    case AS_NAMESPACE_QUALIFIER asBUILD_IN_PROGRESS:
        return "asBUILD_IN_PROGRESS";
    case AS_NAMESPACE_QUALIFIER asINIT_GLOBAL_VARS_FAILED:
        return "asINIT_GLOBAL_VARS_FAILED";
    case AS_NAMESPACE_QUALIFIER asOUT_OF_MEMORY:
        return "asOUT_OF_MEMORY";
    case AS_NAMESPACE_QUALIFIER asMODULE_IS_IN_USE:
        return "asMODULE_IS_IN_USE";

    [[unlikely]] default:
        return string_concat(
            "asERetCodes(",
            std::to_string(static_cast<int>(ret)),
            ')'
        );
    }
}

/**
 * @brief Get current script context from a function called by script
 *
 * @return A pointer to the currently executing context, or null if no context is executing
 */
[[nodiscard]]
inline auto current_context()
    -> AS_NAMESPACE_QUALIFIER asIScriptContext*
{
    return AS_NAMESPACE_QUALIFIER asGetActiveContext();
}

/**
 * @brief Check if the script context has exception
 */
[[nodiscard]]
inline bool has_script_exception(
    AS_NAMESPACE_QUALIFIER asIScriptContext* ctx = current_context()
)
{
    if(!ctx) [[unlikely]]
        return false;

    return ctx->GetState() ==
           AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION;
}

/**
 * @brief Atomic counter for multithreading
 *
 * @note Its initial value will be 1.
 *
 * @details This wraps the `asAtomicInc` and `asAtomicDec`.
 */
class atomic_counter
{
public:
    using value_type = int;

    /**
     * @brief Construct a new atomic counter and set the counter value to 1
     */
    atomic_counter() noexcept
        : m_val(1) {}

    atomic_counter(const atomic_counter&) = default;

    atomic_counter(int val) noexcept
        : m_val(val) {}

    ~atomic_counter() = default;

    atomic_counter& operator=(const atomic_counter&) noexcept = default;

    atomic_counter& operator=(int val) noexcept
    {
        m_val = val;
        return *this;
    }

    bool operator==(const atomic_counter&) const = default;

    int inc() noexcept
    {
        return AS_NAMESPACE_QUALIFIER asAtomicInc(m_val);
    }

    int dec() noexcept
    {
        return AS_NAMESPACE_QUALIFIER asAtomicDec(m_val);
    }

    operator int() const noexcept
    {
        return m_val;
    }

    /**
     * @name Increment and decrement operators
     *
     * Even the prefix increment / decrement will return `int` value directly,
     * which is similar to how the `std::atomic<T>` does.
     */
    /// @{

    int operator++() noexcept
    {
        return inc();
    }

    int operator--() noexcept
    {
        return dec();
    }

    /// @}

    /**
     * @brief Decrease reference count. It will call the destroyer if the count reaches 0.
     */
    template <typename Destroyer, typename... Args>
    decltype(auto) dec_and_try_destroy(Destroyer&& d, Args&&... args)
    {
        if(dec() == 0)
        {
            return std::invoke(
                std::forward<Destroyer>(d),
                std::forward<Args>(args)...
            );
        }
    }

    /**
     * @brief Decrease reference count. It will `delete` the pointer if the count reaches 0.
     */
    template <typename T>
    requires(requires(T* ptr) { delete ptr; })
    void dec_and_try_delete(T* ptr) noexcept
    {
        if(dec() == 0)
        {
            delete ptr;
        }
    }

private:
    int m_val;
};

/**
 * @brief Proxy for the initialization list of AngelScript with repeated values
 *
 * @warning Never use this proxy with a pattern of limited size, e.g., `{int, int}`
 */
class script_init_list_repeat
{
public:
    using size_type = AS_NAMESPACE_QUALIFIER asUINT;

    script_init_list_repeat() = delete;
    script_init_list_repeat(const script_init_list_repeat&) noexcept = default;

    explicit script_init_list_repeat(std::nullptr_t) = delete;

    /**
     * @brief Construct from the initialization list buffer
     *
     * @param list_buf Address of the buffer
     */
    explicit script_init_list_repeat(void* list_buf) noexcept
    {
        assert(list_buf);
        m_size = *static_cast<size_type*>(list_buf);
        m_data = static_cast<std::byte*>(list_buf) + sizeof(size_type);
    }

    /**
     * @brief Construct from the interface for generic calling convention
     *
     * @param gen The interface for the generic calling convention
     * @param idx The parameter index of the list. Usually, this should be 0 for ordinary types and 1 for template classes.
     */
    explicit script_init_list_repeat(
        AS_NAMESPACE_QUALIFIER asIScriptGeneric* gen,
        size_type idx = 0
    )
        : script_init_list_repeat(*(void**)gen->GetAddressOfArg(idx)) {}

    script_init_list_repeat& operator=(const script_init_list_repeat&) noexcept = default;

    bool operator==(const script_init_list_repeat& rhs) const noexcept
    {
        return m_data == rhs.data();
    }

    /**
     * @brief Size of the initialization list
     */
    [[nodiscard]]
    size_type size() const noexcept
    {
        return m_size;
    }

    /**
     * @brief Data address of the elements
     */
    [[nodiscard]]
    void* data() const noexcept
    {
        return m_data;
    }

    /**
     * @brief Revert to raw pointer for forwarding list buffer to another function
     */
    [[nodiscard]]
    void* forward() const noexcept
    {
        return static_cast<std::byte*>(m_data) - sizeof(size_type);
    }

private:
    size_type m_size;
    void* m_data;
};

/**
 * @brief Get string from an enum value
 *
 * @note This function uses compiler extension to get name of enum.
 *       It cannot handle enum value that has same underlying value with another enum.
 *
 * @tparam Value Enum value
 */
template <auto Value>
requires(std::is_enum_v<decltype(Value)>)
constexpr std::string_view static_enum_name() noexcept
{
    std::string_view name;

#if defined(__clang__) || defined(__GNUC__)
    name = __PRETTY_FUNCTION__;

    std::size_t start = name.find("Value = ") + 8;

#    ifdef __clang__
#        define ASBIND20_HAS_STATIC_ENUM_NAME "__PRETTY_FUNCTION__ (Clang)"

    std::size_t end = name.find_last_of(']');
#    else // GCC
#        define ASBIND20_HAS_STATIC_ENUM_NAME "__PRETTY_FUNCTION__ (GCC)"

    std::size_t end = std::min(name.find(';', start), name.find_last_of(']'));
#    endif

    name = std::string_view(name.data() + start, end - start);

#elif defined(_MSC_VER)
#    define ASBIND20_HAS_STATIC_ENUM_NAME "__FUNCSIG__"

    name = __FUNCSIG__;
    std::size_t start = name.find("static_enum_name<") + 17;
    std::size_t end = name.find_last_of('>');
    name = std::string_view(name.data() + start, end - start);

#else
    static_assert(false, "Not supported");

#endif

    // Remove qualifier
    std::size_t qual_end = name.rfind("::");
    if(qual_end != std::string_view::npos)
    {
        qual_end += 2; // skip "::"
        return name.substr(qual_end);
    }

    return name;
}

[[nodiscard]]
inline auto get_default_factory(const AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
    -> AS_NAMESPACE_QUALIFIER asIScriptFunction*
{
    if(!ti) [[unlikely]]
        return nullptr;

    for(AS_NAMESPACE_QUALIFIER asUINT i = 0; i < ti->GetFactoryCount(); ++i)
    {
        AS_NAMESPACE_QUALIFIER asIScriptFunction* func =
            ti->GetFactoryByIndex(i);
        if(func->GetParamCount() == 0)
            return func;
    }

    return nullptr;
}

[[nodiscard]]
inline auto get_default_constructor(const AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
    -> AS_NAMESPACE_QUALIFIER asIScriptFunction*
{
    if(!ti) [[unlikely]]
        return nullptr;

    for(AS_NAMESPACE_QUALIFIER asUINT i = 0; i < ti->GetBehaviourCount(); ++i)
    {
        AS_NAMESPACE_QUALIFIER asEBehaviours beh;
        AS_NAMESPACE_QUALIFIER asIScriptFunction* func =
            ti->GetBehaviourByIndex(i, &beh);
        if(beh == AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT)
        {
            if(func->GetParamCount() == 0)
                return func;
        }
    }

    return nullptr;
}

[[nodiscard]]
inline auto get_weakref_flag(const AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
    -> AS_NAMESPACE_QUALIFIER asIScriptFunction*
{
    if(!ti) [[unlikely]]
        return nullptr;

    for(AS_NAMESPACE_QUALIFIER asUINT i = 0; i < ti->GetBehaviourCount(); ++i)
    {
        AS_NAMESPACE_QUALIFIER asEBehaviours beh;
        AS_NAMESPACE_QUALIFIER asIScriptFunction* func =
            ti->GetBehaviourByIndex(i, &beh);
        if(beh == AS_NAMESPACE_QUALIFIER asBEHAVE_GET_WEAKREF_FLAG)
        {
            if(func->GetParamCount() == 0)
                return func;
        }
    }

    return nullptr;
}

[[nodiscard]]
inline int translate_three_way(std::weak_ordering ord) noexcept
{
    if(ord < 0)
        return -1;
    else if(ord > 0)
        return 1;
    else
        return 0;
}

[[nodiscard]]
inline std::strong_ordering translate_opCmp(int cmp) noexcept
{
    if(cmp < 0)
        return std::strong_ordering::less;
    else if(cmp > 0)
        return std::strong_ordering::greater;
    else
        return std::strong_ordering::equivalent;
}

namespace detail
{
    template <typename Fn, typename Tuple>
    decltype(auto) with_cstr_impl(Fn&& fn, Tuple&& tp)
    {
        return std::apply(std::forward<Fn>(fn), std::forward<Tuple>(tp));
    }

    template <typename Fn, typename Tuple, typename Arg, typename... Args>
    decltype(auto) with_cstr_impl(Fn&& fn, Tuple&& tp, Arg&& arg, Args&&... args)
    {
        if constexpr(std::same_as<std::remove_cvref_t<Arg>, std::string_view>)
        {
            if(arg.data()[arg.size()] == '\0')
            {
                return with_cstr_impl(
                    std::forward<Fn>(fn),
                    std::tuple_cat(
                        tp,
                        std::tuple<const char*>(arg.data())
                    ),
                    std::forward<Args>(args)...
                );
            }
            else
            {
                return with_cstr_impl(
                    std::forward<Fn>(fn),
                    std::tuple_cat(
                        tp,
                        std::tuple<const char*>(std::string(arg).c_str())
                    ),
                    std::forward<Args>(args)...
                );
            }
        }
        else if constexpr(std::same_as<std::remove_cvref_t<Arg>, std::string>)
        {
            return with_cstr_impl(
                std::forward<Fn>(fn),
                std::tuple_cat(
                    tp,
                    std::tuple<const char*>(arg.c_str())
                ),
                std::forward<Args>(args)...
            );
        }
        else
        {
            return with_cstr_impl(
                std::forward<Fn>(fn),
                std::tuple_cat(
                    tp,
                    std::make_tuple<Arg&&>(std::forward<Arg>(arg))
                ),
                std::forward<Args>(args)...
            );
        }
    }
} // namespace detail

/**
 * @brief This function will convert `string` and `string_view` in parameters to null-terminated `const char*`
 *        for APIs receiving C-style string.
 *
 * @details This function will make a copy of string view if it is not null-terminated.
 */
template <typename Fn, typename... Args>
decltype(auto) with_cstr(Fn&& fn, Args&&... args)
{
    return detail::with_cstr_impl(
        std::forward<Fn>(fn),
        std::tuple<>(),
        std::forward<Args>(args)...
    );
}

/**
 * @brief Set the script exception to currently active context.
 *
 * This function has no effect when calling outside an active AngelScript context.
 *
 * @param info Exception information
 */
inline void set_script_exception(const char* info)
{
    AS_NAMESPACE_QUALIFIER asIScriptContext* ctx = current_context();
    if(ctx)
        ctx->SetException(info);
}

inline void set_script_exception(const std::string& info)
{
    set_script_exception(info.c_str());
}

inline void set_script_exception(std::string_view info)
{
    with_cstr(
        [](const char* info)
        { set_script_exception(info); },
        info
    );
}

/**
 * @brief Tools for debugging
 */
namespace debugging
{
    /**
     * @brief Get script section name of function
     *
     * This helper can handle the different interfaces for getting the section name across AngelScript versions.
     *
     * @param func Script function. It cannot be nullptr.
     */
    [[nodiscard]]
    inline const char* get_function_section_name(
        const AS_NAMESPACE_QUALIFIER asIScriptFunction* func
    )
    {
        assert(func != nullptr);

#if ANGELSCRIPT_VERSION >= 23800

        const char* result;
        func->GetDeclaredAt(&result, nullptr, nullptr);
        return result;

#else // 2.37
        return func->GetScriptSectionName();
#endif
    }

    /**
     * @brief GC statistics
     */
    struct gc_statistics
    {
        using value_type = AS_NAMESPACE_QUALIFIER asUINT;

        value_type current_size;
        value_type total_destroyed;
        value_type total_detected;
        value_type new_objects;
        value_type total_new_destroyed;

        bool operator==(const gc_statistics&) const noexcept = default;
    };

    /**
     * @brief Get the GC statistics
     *
     * @param engine Script engine. It cannot be nullptr.
     */
    [[nodiscard]]
    inline gc_statistics get_gc_statistics(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
    {
        assert(engine != nullptr);

        gc_statistics result{};
        engine->GetGCStatistics(
            &result.current_size,
            &result.total_destroyed,
            &result.total_detected,
            &result.new_objects,
            &result.total_new_destroyed
        );
        return result;
    }
} // namespace debugging
} // namespace asbind20

#endif
