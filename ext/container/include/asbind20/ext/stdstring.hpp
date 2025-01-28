#ifndef ASBIND20_EXT_CONTAINER_STDSTRING_HPP
#define ASBIND20_EXT_CONTAINER_STDSTRING_HPP

#pragma once

#include <string>
#include <unordered_map>
#include <asbind20/asbind.hpp>

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
        std::equal_to<>,
        as_allocator<std::pair<const std::string, std::size_t>>>;

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

/**
 * @brief Get offset in byte of a UTF-8 string's nth character
 *
 * @return Offset in bytes, or -1 if `n` is out of range
 */
std::size_t u8_index(std::string_view str, std::size_t n) noexcept;

std::size_t u8_index_r(std::string_view str, std::size_t n) noexcept;

std::size_t u8_strlen(std::string_view str) noexcept;

char32_t u8_bytes_to_int(const char* bytes) noexcept;

unsigned int u8_int_to_bytes(char32_t ch, char* buf);

std::string_view u8_substr(
    std::string_view sv,
    std::size_t pos,
    std::size_t n = std::string_view::npos
);

std::string_view u8_remove_prefix(std::string_view str, std::size_t n);

std::string_view u8_remove_suffix(std::string_view str, std::size_t n);

void u8_replace(std::string& target, std::size_t idx, std::size_t n, std::string_view str);

void string_append_ch(std::string& this_, std::uint32_t ch);

void string_prepend_ch(std::string& this_, std::uint32_t ch);

namespace detail
{
    class const_string_iterator
    {
    public:
        using value_type = std::uint32_t;
        using iterator_category = std::bidirectional_iterator_tag;

        const_string_iterator(std::string_view str, std::size_t offset)
            : m_str(str), m_offset(offset) {}

        const_string_iterator(const const_string_iterator&) = default;

        bool operator==(const const_string_iterator& rhs) const
        {
            return m_str.data() == rhs.m_str.data() && m_offset == rhs.m_offset;
        }

        const_string_iterator& operator++()
        {
            std::size_t next_offset = u8_index(m_str.substr(m_offset), 1);
            if(next_offset == std::size_t(-1))
                m_offset = m_str.size();
            else
                m_offset += next_offset;

            return *this;
        }

        const_string_iterator operator++(int)
        {
            const_string_iterator tmp = *this;
            ++(*this);

            return tmp;
        }

        const_string_iterator& operator--()
        {
            std::size_t prev_offset = u8_index_r(m_str.substr(0, m_offset), 1);
            if(prev_offset == std::size_t(-1))
                prev_offset = 0;
            m_offset = prev_offset;

            return *this;
        }

        const_string_iterator operator--(int)
        {
            const_string_iterator tmp = *this;
            --(*this);

            return tmp;
        }

        std::uint32_t operator*() const noexcept
        {
            if(is_end())
                set_script_exception("invalid iterator");
            return u8_bytes_to_int(m_str.data() + m_offset);
        }

        bool is_end() const
        {
            return m_str.size() == m_offset;
        }

        explicit operator bool() const
        {
            return !is_end();
        }

    private:
        std::string_view m_str;
        std::size_t m_offset;
    };
} // namespace detail

inline detail::const_string_iterator string_cbegin(std::string_view str)
{
    return detail::const_string_iterator(str, 0);
}

inline detail::const_string_iterator string_cend(std::string_view str)
{
    return detail::const_string_iterator(str, str.size());
}

void register_std_string(asIScriptEngine* engine, bool as_default = true, bool generic = has_max_portability());

void register_string_utils(asIScriptEngine* engine, bool generic = has_max_portability());
} // namespace asbind20::ext

#endif
