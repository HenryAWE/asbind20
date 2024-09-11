#ifndef ASBIND20_UTILITY_HPP
#define ASBIND20_UTILITY_HPP

#pragma once

#include <cassert>
#include <string>
#include <utility>
#include <mutex>
#include <compare>
#include <angelscript.h>

namespace asbind20
{
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
    inline void concat_impl(std::string& out, const std::string& str)
    {
        out += str;
    }

    inline void concat_impl(std::string& out, std::string_view sv)
    {
        out.append(sv);
    }

    inline void concat_impl(std::string& out, const char* cstr)
    {
        out.append(cstr);
    }

    inline void concat_impl(std::string& out, char ch)
    {
        out.append(1, ch);
    }

    inline std::size_t concat_size(const std::string& str)
    {
        return str.size();
    }

    inline std::size_t concat_size(std::string_view sv)
    {
        return sv.size();
    }

    inline std::size_t concat_size(const char* cstr)
    {
        return std::char_traits<char>::length(cstr);
    }

    inline std::size_t concat_size(char ch)
    {
        return 1;
    }

    template <typename T>
    concept concat_accepted =
        std::is_same_v<std::remove_cvref_t<T>, std::string> ||
        std::is_same_v<std::remove_cvref_t<T>, std::string_view> ||
        std::is_convertible_v<std::decay_t<T>, const char*> ||
        std::is_same_v<std::remove_cvref_t<T>, char>;

    template <concat_accepted... Args>
    std::string& concat_inplace(std::string& out, Args&&... args)
    {
        assert(out.empty());
        std::size_t sz = 0 + (concat_size(args) + ...);
        out.reserve(sz);

        (concat_impl(out, std::forward<Args>(args)), ...);

        return out;
    }

    template <concat_accepted... Args>
    std::string concat(Args&&... args)
    {
        std::string out;
        concat_inplace(out, std::forward<Args>(args)...);
        return out;
    }
} // namespace detail

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

class [[nodiscard]] reuse_active_context
{
public:
    reuse_active_context() = delete;
    reuse_active_context(const reuse_active_context&) = delete;

    reuse_active_context& operator=(const reuse_active_context&) = delete;

    explicit reuse_active_context(asIScriptEngine* engine)
        : m_ctx(asGetActiveContext()), m_is_nested(false)
    {
        assert(engine != nullptr);
        if(m_ctx)
        {
            if(m_ctx->GetEngine() == engine && m_ctx->PushState() >= 0)
                m_is_nested = true;
            else
                m_ctx = nullptr;
        }

        if(!m_ctx)
        {
            m_ctx = engine->CreateContext();
        }
    }

    ~reuse_active_context()
    {
        if(m_ctx)
        {
            if(m_is_nested)
            {
                asEContextState state = m_ctx->GetState();
                m_ctx->PopState();
                if(state == asEXECUTION_ABORTED)
                    m_ctx->Abort();
            }
            else
                m_ctx->Release();
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

    bool is_nested() const noexcept
    {
        return m_is_nested;
    }

private:
    asIScriptContext* m_ctx;
    bool m_is_nested;
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
} // namespace asbind20

#endif
