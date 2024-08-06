#ifndef ASBIND20_EXT_STDSTRING_HPP
#define ASBIND20_EXT_STDSTRING_HPP

#pragma once

#include <string>
#include <unordered_map>
#include "../bind.hpp"

namespace asbind20::ext
{
class string_factory : public asIStringFactory
{
public:
    struct string_hash
    {
        using is_transparent = void;

        std::size_t operator()(const char* txt) const
        {
            return std::hash<std::string_view>{}(txt);
        }

        std::size_t operator()(std::string_view txt) const
        {
            return std::hash<std::string_view>{}(txt);
        }

        std::size_t operator()(const std::string& txt) const
        {
            return std::hash<std::string>{}(txt);
        }
    };

    using map_t = std::unordered_map<
        std::string,
        std::size_t,
        string_hash,
        std::equal_to<>>;

    const void* GetStringConstant(const char* data, asUINT length) override
    {
        std::lock_guard lock(as_exclusive_lock);

        std::string_view view(data, length);
        auto it = m_cache.find(view);
        if(it != m_cache.end())
            it->second += 1;
        else
        {
            try
            {
                it = m_cache.emplace_hint(
                    it,
                    std::piecewise_construct,
                    std::make_tuple(view),
                    std::make_tuple(1u)
                );
            }
            catch(...)
            {
                if(asIScriptContext* ctx = asGetActiveContext(); ctx)
                    ctx->SetException("Failed to create string");
                return nullptr;
            }
        }

        return &it->first;
    }

    int ReleaseStringConstant(const void* str) override
    {
        auto* ptr = reinterpret_cast<const std::string*>(str);

        if(!ptr) [[unlikely]]
            return asERROR;

        int r = asSUCCESS;

        std::lock_guard lock(as_exclusive_lock);

        auto it = m_cache.find(*ptr);

        if(it == m_cache.end())
            r = asERROR;
        else
        {
            assert(it->second != 0);
            it->second -= 1;
            if(it->second == 0)
                m_cache.erase(it);
        }

        return r;
    }

    int GetRawStringData(const void* str, char* data, asUINT* length) const override
    {
        auto* ptr = reinterpret_cast<const std::string*>(str);

        if(ptr == nullptr)
            return asERROR;

        if(length)
            *length = static_cast<asUINT>(ptr->size());
        if(data)
            ptr->copy(data, ptr->size());
        return asSUCCESS;
    }

    static string_factory& get()
    {
        static string_factory instance{};
        return instance;
    }

private:
    map_t m_cache;
};

void register_std_string(asIScriptEngine* engine, bool as_default = true);
} // namespace asbind20::ext

#endif
