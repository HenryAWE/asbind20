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
#include <string>
#include <utility>
#include <mutex> // IWYU pragma: export `std::lock_guard`
#include <compare>
#include <functional>
#include <type_traits>
#include <concepts>
#include "detail/include_as.hpp"

namespace asbind20
{
namespace detail
{
    // std::is_constructible implicitly requires T to be destructible,
    // which is not necessary for generating a constructor/factory.
    // See: https://stackoverflow.com/questions/28085847/stdis-constructible-on-type-with-non-public-destructor
    template <typename T, typename... Args>
    concept check_placement_new = requires(void* mem) {
        new(mem) T(std::declval<Args>()...);
    };
} // namespace detail

/**
 * @brief Check if a type is constructible without requiring it to be destructible
 */
template <typename T, typename... Args>
struct is_only_constructible :
    public std::bool_constant<detail::check_placement_new<T, Args...>>
{};

template <typename T, typename... Args>
constexpr inline bool is_only_constructible_v = is_only_constructible<T, Args...>::value;

namespace detail
{
    template <typename T>
    class func_traits_impl;

#define ASBIND20_FUNC_TRAITS_IMPL_GLOBAL(noexcept_, ...)             \
    template <typename R, typename... Args>                          \
    class func_traits_impl<R(Args...) noexcept_>                     \
    {                                                                \
    public:                                                          \
        using return_type = R;                                       \
        using args_tuple = std::tuple<Args...>;                      \
        using class_type = void;                                     \
        static constexpr bool is_const_v = false;                    \
        static constexpr bool is_noexcept_v = #noexcept_[0] != '\0'; \
    };                                                               \
    template <typename R, typename... Args>                          \
    class func_traits_impl<R (*)(Args...) noexcept_>                 \
    {                                                                \
    public:                                                          \
        using return_type = R;                                       \
        using args_tuple = std::tuple<Args...>;                      \
        using class_type = void;                                     \
        static constexpr bool is_const_v = false;                    \
        static constexpr bool is_noexcept_v = #noexcept_[0] != '\0'; \
    }

    ASBIND20_FUNC_TRAITS_IMPL_GLOBAL(noexcept);
    ASBIND20_FUNC_TRAITS_IMPL_GLOBAL(, );

#undef ASBIND20_FUNC_TRAITS_IMPL_GLOBAL

#define ASBIND20_FUNC_TRAITS_IMPL_MEMBER(const_, noexcept_)          \
    template <typename R, typename Class, typename... Args>          \
    class func_traits_impl<R (Class::*)(Args...) const_ noexcept_>   \
    {                                                                \
    public:                                                          \
        using return_type = R;                                       \
        using args_tuple = std::tuple<Args...>;                      \
        using class_type = Class;                                    \
        static constexpr bool is_const_v = #const_[0] != '\0';       \
        static constexpr bool is_noexcept_v = #noexcept_[0] != '\0'; \
    }

    ASBIND20_FUNC_TRAITS_IMPL_MEMBER(const, noexcept);
    ASBIND20_FUNC_TRAITS_IMPL_MEMBER(const, );
    ASBIND20_FUNC_TRAITS_IMPL_MEMBER(, noexcept);
    ASBIND20_FUNC_TRAITS_IMPL_MEMBER(, );

#undef ASBIND20_FUNC_TRAITS_IMPL_MEMBER

    template <
        std::size_t Idx,
        typename Tuple,
        bool Valid = (Idx < std::tuple_size_v<Tuple>)>
    struct safe_tuple_elem;

    template <
        std::size_t Idx,
        typename Tuple>
    struct safe_tuple_elem<Idx, Tuple, true>
    {
        using type = std::tuple_element_t<Idx, Tuple>;
    };

    template <
        std::size_t Idx,
        typename Tuple>
    struct safe_tuple_elem<Idx, Tuple, false>
    {
        // Invalid index
        using type = void;
    };

    template <typename std::size_t Idx, typename Tuple>
    using safe_tuple_elem_t = typename safe_tuple_elem<Idx, Tuple>::type;
} // namespace detail

/**
 * @brief Function traits
 *
 * @tparam T Function type
 */
template <typename T>
class function_traits : public detail::func_traits_impl<std::remove_cvref_t<T>>
{
    using my_base = detail::func_traits_impl<std::remove_cvref_t<T>>;

public:
    using type = T;
    using return_type = typename my_base::return_type;
    using args_tuple = typename my_base::args_tuple;
    using class_type = typename my_base::class_type;

    static constexpr bool is_method_v = !std::is_void_v<class_type>;
    using is_method = std::bool_constant<is_method_v>;

    using is_const = std::bool_constant<my_base::is_const_v>;
    using is_noexcept = std::bool_constant<my_base::is_noexcept_v>;

    static constexpr std::size_t arg_count_v = std::tuple_size_v<args_tuple>;
    using arg_count = std::integral_constant<
        std::size_t,
        arg_count_v>;

    template <std::size_t Idx>
    using arg_type = std::tuple_element_t<Idx, args_tuple>;

    /**
     * @brief `void` if `Idx` is invalid
     *
     * @tparam Idx Argument index
     */
    template <std::size_t Idx>
    using arg_type_optional = detail::safe_tuple_elem_t<Idx, args_tuple>;

    using first_arg_type = arg_type_optional<0>;

    using last_arg_type = arg_type_optional<arg_count_v - 1>;
};

namespace detail
{
    template <typename T>
    concept is_native_function_helper = std::is_function_v<T> ||
                                        std::is_function_v<std::remove_pointer_t<T>> ||
                                        std::is_member_function_pointer_v<T>;
} // namespace detail

template <typename T>
concept native_function =
    !std::is_convertible_v<T, AS_NAMESPACE_QUALIFIER asGENFUNC_t> &&
    detail::is_native_function_helper<std::decay_t<T>>;

template <typename Lambda>
concept noncapturing_lambda = requires() {
    { +Lambda{} } -> native_function;
} && std::is_empty_v<Lambda>;

/**
 * @brief Wrap NTTP function pointer as type
 *
 * @tparam Function NTTP function pointer
 */
template <native_function auto Function>
struct fp_wrapper_t
{
    static constexpr auto get() noexcept
    {
        return Function;
    }
};

template <native_function auto Function>
constexpr inline fp_wrapper_t<Function> fp{};

template <typename Func>
auto to_asSFuncPtr(Func f)
    -> AS_NAMESPACE_QUALIFIER asSFuncPtr
{
    // Reference: asFUNCTION and asMETHOD from the AngelScript interface
    if constexpr(std::is_member_function_pointer_v<Func>)
        return AS_NAMESPACE_QUALIFIER asSMethodPtr<sizeof(f)>::Convert(f);
    else
        return AS_NAMESPACE_QUALIFIER asFunctionPtr(f);
}

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
constexpr bool is_void(int type_id) noexcept
{
    return type_id == (AS_NAMESPACE_QUALIFIER asTYPEID_VOID);
}

/**
 * @brief Check if a type id refers to a primitive type
 */
constexpr bool is_primitive_type(int type_id) noexcept
{
    return !(type_id & ~(AS_NAMESPACE_QUALIFIER asTYPEID_MASK_SEQNBR));
}

/**
 * @brief Check if a type id refers to an object handle
 */
constexpr bool is_objhandle(int type_id) noexcept
{
    return type_id & (AS_NAMESPACE_QUALIFIER asTYPEID_OBJHANDLE);
}

/**
 * @brief Get the size of a script type
 *
 * @param type_id AngelScript type id
 */
inline auto sizeof_script_type(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, int type_id
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

    AS_NAMESPACE_QUALIFIER asITypeInfo* ti = engine->GetTypeInfoById(type_id);
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
 * @return std::size_t Bytes copied
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

template <typename T>
concept void_ptr = std::is_pointer_v<std::decay_t<T>> &&
                   std::is_void_v<std::remove_pointer_t<std::decay_t<T>>>;

template <typename... Ts>
class overloaded : public Ts...
{
public:
    using Ts::operator()...;
};

template <typename... Ts>
overloaded(Ts&&...) -> overloaded<Ts...>;

/**
 * @brief Dispatches pointer of primitive values to corresponding type. Similar to the `std::visit`.
 *
 * @param Visitor Callable object that can accept all kind of pointer to primitive types
 * @param type_id AngelScript TypeId
 * @param args Pointers to primitive values
 */
template <typename Visitor, void_ptr... VoidPtrs>
requires(sizeof...(VoidPtrs) > 0)
decltype(auto) visit_primitive_type(Visitor&& vis, int type_id, VoidPtrs... args)
{
    assert(is_primitive_type(type_id) && "Must be a primitive type");
    assert(!is_void(type_id) && "Must not be void");

    auto wrapper = [&]<typename T>(std::in_place_type_t<T>) -> decltype(auto)
    {
        return std::invoke(
            std::forward<Visitor>(vis),
            ((typename std::pointer_traits<VoidPtrs>::template rebind<T>)args)...
        );
    };

#define ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE(as_type_id) \
case as_type_id:                                               \
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
 * @note The object handle will be converted to `void**`, while script class and registered type will retain as `void*`
 *
 * @param Visitor Callable object that can accept all kind of pointer
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

/**
 * @brief Wrapper for `asAcquireExclusiveLock()` and `asReleaseExclusiveLock()`
 */
inline constexpr as_exclusive_lock_t as_exclusive_lock = {};

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

/**
 * @brief Wrapper for `asAcquireSharedLock()` and `asReleaseSharedLock()`
 */
inline constexpr as_shared_lock_t as_shared_lock = {};

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
 * @return std::string String representation of the state.
 *                     If the state value is invalid, the result will be "asEContextState({state})",
 *                     e.g. "asEContextState(-1)".
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
        using namespace std::literals;
        return string_concat(
            "asEContextState("sv,
            std::to_string(static_cast<int>(state)),
            ')'
        );
    }
}

/**
 * @brief Convert return code to string
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
        using namespace std::literals;
        return string_concat(
            "asERetCodes("sv,
            std::to_string(static_cast<int>(ret)),
            ')'
        );
    }
}

namespace meta
{
    template <std::size_t Size>
    class fixed_string
    {
    public:
        using value_type = char;
        using size_type = std::size_t;

        /**
         * @brief INTERNAL DATA. DO NOT USE!
         *
         * This member is exposed for satisfying the NTTP requirements of C++.
         *
         * @note It includes '\0'
         */
        char internal_data[Size + 1] = {};

        constexpr fixed_string(const fixed_string&) noexcept = default;

        template <std::convertible_to<char>... Chars>
        requires(sizeof...(Chars) == Size)
        explicit constexpr fixed_string(Chars... chs)
            : internal_data{static_cast<char>(chs)..., '\0'}
        {}

        constexpr fixed_string(const char (&str)[Size + 1])
        {
            for(size_type i = 0; i < Size; ++i)
            {
                internal_data[i] = str[i];
            }
            internal_data[Size] = '\0';
        }

        constexpr bool operator==(const fixed_string&) const noexcept = default;

        template <std::size_t N>
        requires(N != Size)
        constexpr bool operator==(const fixed_string<N>&) const noexcept
        {
            return false;
        }

        static constexpr size_type size() noexcept
        {
            return Size;
        };

        static constexpr bool empty() noexcept
        {
            return Size == 0;
        }

        constexpr const value_type* data() const noexcept
        {
            return internal_data;
        }

        constexpr const value_type* c_str() const noexcept
        {
            return data();
        }

        constexpr operator const char*() const noexcept
        {
            return c_str();
        }

        constexpr std::string_view view() const noexcept
        {
            return std::string_view(data(), size());
        }

        constexpr operator std::string_view() const noexcept
        {
            return view();
        }
    };

    template <std::convertible_to<char>... Chars>
    fixed_string(Chars...) -> fixed_string<sizeof...(Chars)>;

    template <std::size_t N>
    fixed_string(const char (&str)[N]) -> fixed_string<N - 1>;

    template <std::size_t SizeL, std::size_t SizeR>
    constexpr auto operator+(const fixed_string<SizeL>& lhs, const fixed_string<SizeR>& rhs) -> fixed_string<SizeL + SizeR>
    {
        if constexpr(SizeL == 0)
            return rhs;
        else if constexpr(SizeR == 0)
            return lhs;
        else
        {
            return [&]<std::size_t... I1, std::size_t... I2>(std::index_sequence<I1...>, std::index_sequence<I2...>)
            {
                return fixed_string<SizeL + SizeR>(lhs.internal_data[I1]..., rhs.internal_data[I2]...);
            }(std::make_index_sequence<SizeL>(), std::make_index_sequence<SizeR>());
        }
    }
} // namespace meta

/**
 * @brief Smart pointer for script object
 */
class script_object
{
public:
    using handle_type = AS_NAMESPACE_QUALIFIER asIScriptObject*;

    script_object() noexcept = default;

    script_object(script_object&& other) noexcept
        : m_obj(std::exchange(other.m_obj, nullptr)) {}

    script_object(const script_object&) = delete;

    explicit script_object(handle_type obj)
        : m_obj(obj)
    {
        if(m_obj)
            m_obj->AddRef();
    }

    ~script_object()
    {
        reset(nullptr);
    }

    handle_type get() const noexcept
    {
        return m_obj;
    }

    explicit operator handle_type() const noexcept
    {
        return get();
    }

    handle_type operator->() const noexcept
    {
        return get();
    }

    /**
     * @brief Release without decreasing reference count
     *
     * @warning USE WITH CAUTION!
     *
     * @return Previously stored object
     */
    handle_type release() noexcept
    {
        return std::exchange(m_obj, nullptr);
    }

    /**
     * @brief Reset object the null pointer
     *
     * @return int Reference count after releasing the object
     */
    int reset(std::nullptr_t = nullptr) noexcept
    {
        int prev_refcount = 0;
        if(m_obj)
        {
            prev_refcount = m_obj->Release();
            m_obj = nullptr;
        }

        return prev_refcount;
    }

    /**
     * @brief Reset object
     *
     * @param obj New object to store
     *
     * @return Reference count after releasing the object
     */
    int reset(handle_type obj)
    {
        int prev_refcount = reset(nullptr);

        if(obj)
        {
            obj->AddRef();
            m_obj = obj;
        }

        return prev_refcount;
    }

private:
    asIScriptObject* m_obj = nullptr;
};

/**
 * @brief Get current script context from a function called by script
 *
 * @return A pointer to the currently executing context, or null if no context is executing
 */
inline auto current_context()
    -> AS_NAMESPACE_QUALIFIER asIScriptContext*
{
    return AS_NAMESPACE_QUALIFIER asGetActiveContext();
}

/**
 * @brief RAII helper for reusing active script context.
 *
 * It will fallback to request context from the engine.
 */
class [[nodiscard]] reuse_active_context
{
public:
    using handle_type = asIScriptContext*;

    reuse_active_context() = delete;
    reuse_active_context(const reuse_active_context&) = delete;

    reuse_active_context& operator=(const reuse_active_context&) = delete;

    explicit reuse_active_context(
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, bool propagate_error = true
    )
        : m_engine(engine), m_propagate_error(propagate_error)
    {
        assert(m_engine != nullptr);

        m_ctx = current_context();
        if(m_ctx)
        {
            if(m_ctx->GetEngine() == engine && m_ctx->PushState() >= 0)
                m_is_nested = true;
            else
                m_ctx = nullptr;
        }

        if(!m_ctx)
        {
            m_ctx = engine->RequestContext();
        }
    }

    ~reuse_active_context()
    {
        if(m_ctx)
        {
            if(m_is_nested)
            {
                if(m_propagate_error)
                {
                    std::string ex;
                    AS_NAMESPACE_QUALIFIER asEContextState state =
                        m_ctx->GetState();
                    if(state == AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION)
                        ex = m_ctx->GetExceptionString();

                    m_ctx->PopState();

                    switch(state)
                    {
                    case AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION:
                        m_ctx->SetException(ex.c_str());
                        break;

                    case AS_NAMESPACE_QUALIFIER asEXECUTION_ABORTED:
                        m_ctx->Abort();
                        break;

                    default:
                        break;
                    }
                }
                else
                    m_ctx->PopState();
            }
            else
                m_engine->ReturnContext(m_ctx);
        }
    }

    handle_type get() const noexcept
    {
        return m_ctx;
    }

    operator handle_type() const noexcept
    {
        return get();
    }

    handle_type operator->() const noexcept
    {
        return get();
    }

    /**
     * @brief Returns true if current context is reused.
     */
    bool is_nested() const noexcept
    {
        return m_is_nested;
    }

    bool will_propagate_error() const noexcept
    {
        return m_propagate_error;
    }

private:
    AS_NAMESPACE_QUALIFIER asIScriptEngine* m_engine = nullptr;
    handle_type m_ctx = nullptr;
    bool m_is_nested = false;
    bool m_propagate_error = true;
};

/**
 * @brief RAII helper for requesting script context from the engine
 */
class [[nodiscard]] request_context
{
public:
    using handle_type = AS_NAMESPACE_QUALIFIER asIScriptContext*;

    request_context() = delete;
    request_context(const request_context&) = delete;

    request_context& operator=(const request_context&) = delete;

    explicit request_context(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
        : m_engine(engine)
    {
        assert(m_engine != nullptr);
        m_ctx = m_engine->RequestContext();
    }

    ~request_context()
    {
        if(m_ctx)
            m_engine->ReturnContext(m_ctx);
    }

    handle_type get() const noexcept
    {
        return m_ctx;
    }

    operator handle_type() const noexcept
    {
        return get();
    }

    handle_type operator->() const noexcept
    {
        return get();
    }

private:
    AS_NAMESPACE_QUALIFIER asIScriptEngine* m_engine = nullptr;
    handle_type m_ctx = nullptr;
};

/**
 * @brief Script engine manager
 */
class script_engine
{
public:
    using handle_type = AS_NAMESPACE_QUALIFIER asIScriptEngine*;

    script_engine() noexcept
        : m_engine(nullptr) {}

    script_engine(const script_engine&) = delete;

    script_engine(script_engine&&) noexcept
        : m_engine(std::exchange(m_engine, nullptr)) {}

    explicit script_engine(handle_type engine) noexcept
        : m_engine(engine) {}

    script_engine& operator=(const script_engine&) = delete;

    script_engine& operator=(script_engine&& other) noexcept
    {
        if(this != &other)
            reset(other.release());
        return *this;
    }

    ~script_engine()
    {
        reset();
    }

    handle_type get() const noexcept
    {
        return m_engine;
    }

    operator handle_type() const noexcept
    {
        return get();
    }

    handle_type operator->() const noexcept
    {
        return get();
    }

    handle_type release() noexcept
    {
        return std::exchange(m_engine, nullptr);
    }

    void reset(handle_type engine = nullptr) noexcept
    {
        if(m_engine)
            m_engine->ShutDownAndRelease();
        m_engine = engine;
    }

private:
    handle_type m_engine;
};

/**
 * @brief Create an AngelScript engine
 */
inline script_engine make_script_engine(
    AS_NAMESPACE_QUALIFIER asDWORD version = ANGELSCRIPT_VERSION
)
{
    return script_engine(
        AS_NAMESPACE_QUALIFIER asCreateScriptEngine(version)
    );
}

/**
 * @brief Wrap `asAllocMem()` and `asFreeMem()` as a C++ allocator
 */
template <typename T>
class as_allocator
{
public:
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;

    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::true_type;

    as_allocator() noexcept = default;

    template <typename U>
    as_allocator(const as_allocator<U>&) noexcept
    {}

    as_allocator& operator=(const as_allocator&) = default;

    ~as_allocator() = default;

    static pointer allocate(size_type n)
    {
        size_type size = n * sizeof(value_type);
        return (pointer)(AS_NAMESPACE_QUALIFIER asAllocMem(size));
    }

    static void deallocate(pointer mem, size_type n) noexcept
    {
        (void)n; // unused
        AS_NAMESPACE_QUALIFIER asFreeMem(static_cast<void*>(mem));
    }
};

/**
 * @brief Proxy for the initialization list of AngelScript with repeated values
 *
 * Official documentation:
 * https://www.angelcode.com/angelscript/sdk/docs/manual/doc_reg_basicref.html#doc_reg_basicref_4
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

    explicit script_init_list_repeat(void* list_buf) noexcept
    {
        assert(list_buf);
        m_size = *static_cast<size_type*>(list_buf);
        m_data = static_cast<std::byte*>(list_buf) + sizeof(size_type);
    }

    /**
     * @brief Create a wrapper for script initialization list from for generic calling convention
     *
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

    size_type size() const noexcept
    {
        return m_size;
    }

    void* data() const noexcept
    {
        return m_data;
    }

    /**
     * @brief Revert to raw pointer for forwarding list to another function
     */
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

namespace meta
{
    template <auto Value>
    auto fixed_enum_name() noexcept
    {
        constexpr std::string_view name_view = static_enum_name<Value>();
        constexpr std::size_t size = name_view.size();

        return [&]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            return fixed_string<size>(name_view[Is]...);
        }(std::make_index_sequence<size>());
    }
} // namespace meta

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

template <typename T>
requires(std::is_arithmetic_v<T>)
auto name_of() noexcept
{
    if constexpr(std::same_as<T, bool>)
        return meta::fixed_string("bool");
    else if constexpr(std::integral<T>)
    {
        if constexpr(std::is_unsigned_v<T>)
        {
            if constexpr(sizeof(T) == 1)
                return meta::fixed_string("uint8");
            else if constexpr(sizeof(T) == 2)
                return meta::fixed_string("uint16");
            else if constexpr(sizeof(T) == 4)
                return meta::fixed_string("uint");
            else if constexpr(sizeof(T) == 8)
                return meta::fixed_string("uint64");
            else
                static_assert(!sizeof(T), "Invalid integral");
        }
        else if constexpr(std::is_signed_v<T>)
        {
            if constexpr(sizeof(T) == 1)
                return meta::fixed_string("int8");
            else if constexpr(sizeof(T) == 2)
                return meta::fixed_string("int16");
            else if constexpr(sizeof(T) == 4)
                return meta::fixed_string("int");
            else if constexpr(sizeof(T) == 8)
                return meta::fixed_string("int64");
            else
                static_assert(!sizeof(T), "Invalid integral");
        }
        else // Neither signed nor unsigned
            static_assert(!sizeof(T), "Invalid integral");
    }
    else if constexpr(std::floating_point<T>)
    {
        if constexpr(std::same_as<T, float>)
            return meta::fixed_string("float");
        else if constexpr(std::same_as<T, double>)
            return meta::fixed_string("double");
        else
            static_assert(!sizeof(T), "Invalid floating point");
    }
    else
        static_assert(!sizeof(T), "Invalid arithmetic");
}

template <typename T>
concept has_static_name =
    std::is_arithmetic_v<T> &&
    !std::same_as<T, char>;

inline auto get_default_factory(AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
    -> AS_NAMESPACE_QUALIFIER asIScriptFunction*
{
    assert(ti != nullptr);

    for(AS_NAMESPACE_QUALIFIER asUINT i = 0; i < ti->GetFactoryCount(); ++i)
    {
        AS_NAMESPACE_QUALIFIER asIScriptFunction* func =
            ti->GetFactoryByIndex(i);
        if(func->GetParamCount() == 0)
            return func;
    }

    return nullptr;
}

inline auto get_default_constructor(AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
    -> AS_NAMESPACE_QUALIFIER asIScriptFunction*
{
    assert(ti != nullptr);

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

inline int translate_three_way(std::weak_ordering ord) noexcept
{
    if(ord < 0)
        return -1;
    else if(ord > 0)
        return 1;
    else
        return 0;
}

inline std::strong_ordering translate_opCmp(int cmp) noexcept
{
    if(cmp < 0)
        return std::strong_ordering::less;
    else if(cmp > 0)
        return std::strong_ordering::greater;
    else
        return std::strong_ordering::equivalent;
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
} // namespace asbind20

#endif
