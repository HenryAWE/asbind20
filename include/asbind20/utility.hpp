#ifndef ASBIND20_UTILITY_HPP
#define ASBIND20_UTILITY_HPP

#pragma once

#include <cassert>
#include <string>
#include <utility>
#include <angelscript.h>

namespace asbind20
{
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

template <typename T, typename Class>
std::size_t member_offset(T Class::*mp) noexcept
{
    Class* p = nullptr;
    return std::size_t(std::addressof(p->*mp)) - std::size_t(p);
}
} // namespace asbind20

#endif
