#ifndef ASBIND20_UTILITY_HPP
#define ASBIND20_UTILITY_HPP

#pragma once

#include <cassert>
#include <cstddef>
#include <string>
#include <utility>
#include <mutex>
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

    template <typename T>
    concept callable_class = std::is_class_v<T>&& requires()
    {
        &T::operator();
    };

    template <typename T>
    requires callable_class<T>
    class func_traits_impl<T> : public func_traits_impl<decltype(&T::operator())>
    {
    private:
        using my_base = func_traits_impl<decltype(&T::operator())>;

    public:
        using return_type = typename my_base::return_type;
        using args_tuple = typename my_base::args_tuple;
        using class_type = typename my_base::class_type;
    };

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

    // `void` if `Idx` is invalid
    template <std::size_t Idx>
    using arg_type_optional = detail::safe_tuple_elem_t<Idx, args_tuple>;

    using first_arg_type = arg_type_optional<0>;

    using last_arg_type = arg_type_optional<arg_count_v - 1>;
};

namespace detail
{
    template <typename T>
    constexpr T& refptr_helper_ref(T& ref) noexcept
    {
        return ref;
    }

    template <typename T>
    constexpr void refptr_helper_ref(T&&) = delete;

    template <typename T, typename U>
    concept refptr_check_helper_ref = requires() {
        refptr_helper_ref<T>(std::declval<U>());
    };
} // namespace detail

/**
 * @brief Wrapper for interchanging between reference and pointer. Similar to the `std::reference_wrapper<T>`
 */
template <typename T>
class refptr_wrapper
{
public:
    using type = T;

    template <typename U>
    requires(detail::refptr_check_helper_ref<T, U> && !std::is_same_v<refptr_wrapper, std::remove_cvref_t<U>>)
    constexpr refptr_wrapper(U&& ref) noexcept(noexcept(detail::refptr_helper_ref<T>(std::forward<U>(ref))))
        : m_ptr(std::addressof(detail::refptr_helper_ref<T>(std::forward<U>(ref))))
    {}

    constexpr refptr_wrapper(T* ptr) noexcept
        : m_ptr(ptr) {}

    constexpr refptr_wrapper(const refptr_wrapper&) noexcept = default;

    constexpr refptr_wrapper& operator=(const refptr_wrapper&) noexcept = default;

    constexpr operator T&() const noexcept
    {
        return *m_ptr;
    }

    constexpr operator T*() const noexcept
    {
        return m_ptr;
    }

    constexpr T& get() const noexcept
    {
        return *m_ptr;
    }

    constexpr T* get_ptr() const noexcept
    {
        return m_ptr;
    }

private:
    T* m_ptr;
};

template <typename T>
refptr_wrapper(T&) -> refptr_wrapper<T>;

template <typename T>
refptr_wrapper(T*) -> refptr_wrapper<T>;

template <int TypeId>
requires(!(TypeId & ~asTYPEID_MASK_SEQNBR))
struct primitive_type_of;

#define ASBIND20_UTILITY_DEFINE_PRIMITIVE_TYPE_OF(as_type_id, cpp_type, script_decl) \
    template <>                                                                      \
    struct primitive_type_of<as_type_id>                                             \
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
 * @brief Check if a type id refers to a primitive type
 */
inline bool is_primitive_type(int type_id) noexcept
{
    return !(type_id & ~asTYPEID_MASK_SEQNBR);
}

/**
 * @brief Check if a type id refers to an object handle
 */
inline bool is_objhandle(int type_id) noexcept
{
    return type_id & asTYPEID_OBJHANDLE;
}

/**
 * @brief Get the size of a script type
 *
 * @param type_id AngelScript type id
 * @return asUINT
 */
asUINT sizeof_script_type(asIScriptEngine* engine, int type_id);

/**
 * @brief Extracts the contents from a script string without knowing the underlying type
 *
 * @param factory The string factory
 * @param str The pointer to script string
 *
 * @return std::string Result string
 */
std::string extract_string(asIStringFactory* factory, void* str);

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
std::size_t copy_primitive_value(void* dst, const void* src, int type_id);

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
    assert(type_id != asTYPEID_VOID && "Must not be void");

    auto wrapper = [&]<typename T>(std::in_place_type_t<T>) -> decltype(auto)
    {
        return std::invoke(
            std::forward<Visitor>(vis),
            ((typename std::pointer_traits<VoidPtrs>::template rebind<T>)args)...
        );
    };

#define ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE(as_type_id) \
case as_type_id:                                               \
    return wrapper(std::in_place_type<primitive_type_of_t<as_type_id>>)

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
    else if(type_id & asTYPEID_OBJHANDLE)
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
        asAcquireExclusiveLock();
    }

    static void unlock()
    {
        asReleaseExclusiveLock();
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
        asAcquireSharedLock();
    }

    static void unlock()
    {
        asReleaseSharedLock();
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

    constexpr void concat_impl(std::string& out, std::string_view sv)
    {
        out.append(sv);
    }

    constexpr void concat_impl(std::string& out, const char* cstr)
    {
        out.append(cstr);
    }

    constexpr void concat_impl(std::string& out, char ch)
    {
        out.append(1, ch);
    }

    constexpr std::size_t concat_size(const std::string& str)
    {
        return str.size();
    }

    constexpr std::size_t concat_size(std::string_view sv)
    {
        return sv.size();
    }

    constexpr std::size_t concat_size(const char* cstr)
    {
        return std::char_traits<char>::length(cstr);
    }

    constexpr std::size_t concat_size(char ch)
    {
        (void)ch;
        return 1;
    }

    template <typename T>
    concept concat_accepted =
        std::is_same_v<std::remove_cvref_t<T>, std::string> ||
        std::is_same_v<std::remove_cvref_t<T>, std::string_view> ||
        std::is_convertible_v<std::decay_t<T>, const char*> ||
        std::is_same_v<std::remove_cvref_t<T>, char>;
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
    assert(out.empty());
    std::size_t sz = 0 + (detail::concat_size(args) + ...);
    out.reserve(sz);

    (detail::concat_impl(out, std::forward<Args>(args)), ...);

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
 * @brief Smart pointer for script object
 */
class script_object
{
public:
    script_object() noexcept = default;

    script_object(script_object&& other) noexcept
        : m_obj(std::exchange(other.m_obj, nullptr)) {}

    script_object(const script_object&) = delete;

    explicit script_object(asIScriptObject* obj)
        : m_obj(obj)
    {
        if(m_obj)
            m_obj->AddRef();
    }

    ~script_object()
    {
        reset(nullptr);
    }

    asIScriptObject* get() const noexcept
    {
        return m_obj;
    }

    explicit operator asIScriptObject*() const noexcept
    {
        return get();
    }

    /**
     * @brief Release without decreasing reference count
     *
     * @warning USE WITH CAUTION!
     *
     * @return asIScriptObject* Previously stored object
     */
    asIScriptObject* release() noexcept
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
     * @return int Reference count after releasing the object
     */
    int reset(asIScriptObject* obj)
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
 * @brief RAII helper for reusing active script context.
 *
 * It will fallback to request context from the engine.
 */
class [[nodiscard]] reuse_active_context
{
public:
    reuse_active_context() = delete;
    reuse_active_context(const reuse_active_context&) = delete;

    reuse_active_context& operator=(const reuse_active_context&) = delete;

    explicit reuse_active_context(asIScriptEngine* engine, bool propagate_error = true)
        : m_engine(engine), m_propagate_error(propagate_error)
    {
        assert(m_engine != nullptr);

        m_ctx = asGetActiveContext();
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
                    asEContextState state = m_ctx->GetState();
                    if(state == asEXECUTION_EXCEPTION)
                        ex = m_ctx->GetExceptionString();

                    m_ctx->PopState();

                    switch(state)
                    {
                    case asEXECUTION_EXCEPTION:
                        m_ctx->SetException(ex.c_str());
                        break;

                    case asEXECUTION_ABORTED:
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

    asIScriptContext* get() const noexcept
    {
        return m_ctx;
    }

    operator asIScriptContext*() const noexcept
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
    asIScriptEngine* m_engine = nullptr;
    asIScriptContext* m_ctx = nullptr;
    bool m_is_nested = false;
    bool m_propagate_error = true;
};

/**
 * @brief RAII helper for requesting script context from the engine
 */
class [[nodiscard]] request_context
{
public:
    request_context() = delete;
    request_context(const request_context&) = delete;

    request_context& operator=(const request_context&) = delete;

    explicit request_context(asIScriptEngine* engine)
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

    asIScriptContext* get() const noexcept
    {
        return m_ctx;
    }

    operator asIScriptContext*() const noexcept
    {
        return get();
    }

private:
    asIScriptEngine* m_engine = nullptr;
    asIScriptContext* m_ctx = nullptr;
};

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
        return (pointer)asAllocMem(n * sizeof(value_type));
    }

    static void deallocate(pointer mem, size_type n) noexcept
    {
        (void)n; // unused
        asFreeMem(mem);
    }
};

// Tools for initialization list
// Ref: https://www.angelcode.com/angelscript/sdk/docs/manual/doc_reg_basicref.html#doc_reg_basicref_4

/**
 * @brief Wrapper for the initialization list of AngelScript with repeated values
 */
class script_init_list_repeat
{
public:
    using size_type = asUINT;

    script_init_list_repeat() = delete;
    script_init_list_repeat(const script_init_list_repeat&) noexcept = default;

    explicit script_init_list_repeat(void* list_buf) noexcept
    {
        assert(list_buf);
        m_size = *static_cast<size_type*>(list_buf);
        m_data = static_cast<std::byte*>(list_buf) + sizeof(size_type);
    }

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
 * @brief Get offset from a member pointer
 *
 * @note This function is implemented by undefined behavior but is expected to work on most platforms
 */
template <typename T, typename Class>
std::size_t member_offset(T Class::*mp) noexcept
{
    Class* p = nullptr;
    return std::size_t(std::addressof(p->*mp)) - std::size_t(p);
}

asIScriptFunction* get_default_factory(asITypeInfo* ti);

asIScriptFunction* get_default_constructor(asITypeInfo* ti);

int translate_three_way(std::weak_ordering ord) noexcept;
std::strong_ordering translate_opCmp(int cmp) noexcept;

void set_script_exception(asIScriptContext* ctx, const char* info);
void set_script_exception(asIScriptContext* ctx, const std::string& info);

/**
 * @brief Set the script exception to currently active context.
 *
 * This function has no effect when calling outside an active AngelScript context.
 *
 * @param info Exception information
 */
void set_script_exception(const char* info);
void set_script_exception(const std::string& info);

template <typename T>
struct type_traits;

template <>
struct type_traits<std::byte>
{
    static int set_arg(asIScriptContext* ctx, asUINT arg, std::byte val)
    {
        return ctx->SetArgByte(arg, static_cast<asBYTE>(val));
    }

    static std::byte get_arg(asIScriptGeneric* gen, asUINT arg)
    {
        return static_cast<std::byte>(gen->GetArgByte(arg));
    }

    static int set_return(asIScriptGeneric* gen, std::byte val)
    {
        return gen->SetReturnByte(static_cast<asBYTE>(val));
    }

    static std::byte get_return(asIScriptContext* ctx)
    {
        return static_cast<std::byte>(ctx->GetReturnByte());
    }
};

template <>
struct type_traits<script_object>
{
    static int set_arg(asIScriptContext* ctx, asUINT arg, const script_object& val)
    {
        return ctx->SetArgObject(arg, val.get());
    }

    static script_object get_arg(asIScriptGeneric* gen, asUINT arg)
    {
        return script_object(static_cast<asIScriptObject*>(gen->GetArgObject(arg)));
    }

    static int set_return(asIScriptGeneric* gen, const script_object& val)
    {
        return gen->SetReturnObject(val.get());
    }

    static script_object get_return(asIScriptContext* ctx)
    {
        return script_object(static_cast<asIScriptObject*>(ctx->GetReturnObject()));
    }
};

template <>
struct type_traits<const script_object> : public type_traits<script_object>
{};

/**
 * @brief Execute a piece of AngelScript code
 */
int exec(
    asIScriptEngine* engine,
    std::string_view code,
    asIScriptContext* ctx = nullptr
);
} // namespace asbind20

#endif
