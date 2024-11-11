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
template <typename Class, typename... Args>
concept newable = requires(Args... args) {
    new Class(args...);
};

template <int TypeId>
requires(!(TypeId & ~asTYPEID_MASK_SEQNBR))
struct primitive_type_of
{
    static_assert(TypeId > asTYPEID_DOUBLE);
    using type = int; // enums
    static constexpr char decl[] = "int"; // placeholder
};

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

template <typename Visitor, typename... Args>
requires(sizeof...(Args) > 0 && (std::is_void_v<Args> && ...))
decltype(auto) visit_primitive_type(Visitor&& vis, int type_id, Args*... args)
{
    assert(!(type_id & ~asTYPEID_MASK_SEQNBR));
    assert(type_id != asTYPEID_VOID);

    auto wrapper = [&]<typename T>() -> decltype(auto)
    {
        return std::invoke(
            std::forward<Visitor>(vis),
            ((typename std::pointer_traits<Args*>::template rebind<T>)args)...
        );
    };

#define ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE(as_type_id) \
case as_type_id: return wrapper.template operator()<primitive_type_of_t<as_type_id>>();

    switch(type_id)
    {
        ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE(asTYPEID_BOOL);
        ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE(asTYPEID_INT8);
        ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE(asTYPEID_INT16);
        ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE(asTYPEID_INT32);
        ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE(asTYPEID_INT64);
        ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE(asTYPEID_UINT8);
        ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE(asTYPEID_UINT16);
        ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE(asTYPEID_UINT32);
        ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE(asTYPEID_UINT64);
        ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE(asTYPEID_FLOAT);
        ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE(asTYPEID_DOUBLE);

    default: /* enums */
        return wrapper.template operator()<int>();
    }

#undef ASBIND20_UTILITY_VISIT_PRIMITIVE_TYPE_CASE
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
 *
 * @see as_exclusive_lock_t
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
        return 1;
    }

    template <typename T>
    concept concat_accepted =
        std::is_same_v<std::remove_cvref_t<T>, std::string> ||
        std::is_same_v<std::remove_cvref_t<T>, std::string_view> ||
        std::is_convertible_v<std::decay_t<T>, const char*> ||
        std::is_same_v<std::remove_cvref_t<T>, char>;
} // namespace detail

template <detail::concat_accepted... Args>
constexpr std::string& string_concat_inplace(std::string& out, Args&&... args)
{
    assert(out.empty());
    std::size_t sz = 0 + (detail::concat_size(args) + ...);
    out.reserve(sz);

    (detail::concat_impl(out, std::forward<Args>(args)), ...);

    return out;
}

template <detail::concat_accepted... Args>
constexpr std::string string_concat(Args&&... args)
{
    std::string out;
    string_concat_inplace(out, std::forward<Args>(args)...);
    return out;
}


/**
 * @brief Wrapper for script object, similar to a `std::unique_ptr`
 */
class object
{
public:
    object() noexcept = default;

    object(object&& other) noexcept
        : m_obj(std::exchange(other.m_obj, nullptr)) {}

    object(const object&) = delete;

    object(asIScriptObject* obj, bool addref = true)
        : m_obj(obj)
    {
        if(m_obj && addref)
            m_obj->AddRef();
    }

    asIScriptObject* get() const noexcept
    {
        return m_obj;
    }

    operator asIScriptObject*() const noexcept
    {
        return get();
    }

    asIScriptObject* release() noexcept
    {
        return std::exchange(m_obj, nullptr);
    }

    void reset(std::nullptr_t) noexcept
    {
        if(m_obj)
            m_obj->Release();
        release();
    }

    void reset(asIScriptObject* obj) noexcept
    {
        reset(nullptr);
        if(obj)
        {
            obj->AddRef();
            m_obj = obj;
        }
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

    request_context(asIScriptEngine* engine)
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
    as_allocator(const as_allocator<U>&) noexcept {};

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

/**
 * @brief Wrapper for the initialization list of AngelScript
 */
class script_init_list
{
public:
    using size_type = asUINT;

    script_init_list() = delete;
    script_init_list(const script_init_list&) noexcept = default;

    explicit script_init_list(void* list_buf) noexcept
    {
        assert(list_buf);
        m_size = *static_cast<size_type*>(list_buf);
        m_data = static_cast<std::byte*>(list_buf) + sizeof(size_type);
    }

    script_init_list& operator=(const script_init_list&) noexcept = default;

    bool operator==(const script_init_list& rhs) const noexcept
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
     * @brief Revert to raw pointer for forwarding list to a script function
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
} // namespace asbind20

#endif
