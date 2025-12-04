/**
 * @file strutil.hpp
 * @author HenryAWE
 * @brief String utilities
 */

#ifndef ASBIND20_DETAIL_STRUTIL_HPP
#define ASBIND20_DETAIL_STRUTIL_HPP

#pragma once

#include <iosfwd>
#include <cassert>
#include <string>
#include <string_view>

namespace asbind20
{
namespace util
{
    /**
     * @brief String view with a guaranteed zero at the end
     */
    class cstring_view
    {
    public:
        using size_type = std::size_t;
        using traits_type = std::char_traits<char>;

        constexpr cstring_view() noexcept
            : m_cstr(nullptr), m_size(0) {}

        constexpr cstring_view(const cstring_view&) noexcept = default;

        constexpr cstring_view(const char* cstr, size_type sz) noexcept
            : m_cstr(cstr),
              m_size(sz)
        {
            assert(cstr[sz] == '\0');
        }

        constexpr cstring_view(const std::string& str) noexcept
            : m_cstr(str.c_str()), m_size(str.size()) {}

        constexpr cstring_view(const char* str) noexcept
            : m_cstr(str),
            m_size(std::char_traits<char>::length(str)) {}

        template <std::size_t N>
        constexpr explicit cstring_view(const char (&arr)[N]) noexcept
            : cstring_view(arr, N - 1)
        {}

        constexpr ~cstring_view() = default;

        cstring_view& operator=(const cstring_view&) noexcept = default;

        constexpr int compare(const cstring_view& rhs) const noexcept
        {
            return std::string_view(*this).compare(
                std::string_view(rhs)
            );
        }

        constexpr std::strong_ordering operator<=>(
            const cstring_view& rhs
        ) const noexcept
        {
            return compare(rhs) <=> 0;
        }

        constexpr bool operator==(const cstring_view& rhs) const noexcept
        {
            if(size() != rhs.size())
                return false;
            return compare(rhs) == 0;
        }

        constexpr friend bool operator==(cstring_view lhs, std::string_view rhs) noexcept
        {
            return std::string_view(lhs) == rhs;
        }

        constexpr friend bool operator==(std::string_view lhs, cstring_view rhs) noexcept
        {
            return lhs == std::string_view(rhs);
        }

        constexpr friend bool operator==(cstring_view lhs, const char* rhs) noexcept
        {
            return std::string_view(lhs) == rhs;
        }

        constexpr friend bool operator==(const char* lhs, cstring_view rhs) noexcept
        {
            return lhs == std::string_view(rhs);
        }

        constexpr size_type size() const noexcept
        {
            return m_size;
        }

        constexpr const char* c_str() const
        {
            return m_cstr;
        }

        [[nodiscard]]
        constexpr bool empty() const noexcept
        {
            return m_size == 0;
        }

        constexpr operator std::string_view() const noexcept
        {
            return std::string_view(m_cstr, m_size);
        }

        constexpr void remove_prefix(size_type n) noexcept
        {
            assert(n <= size());
            m_cstr += n;
            m_size -= n;
        }

        friend std::ostream& operator<<(std::ostream& os, cstring_view csv)
        {
            return os << std::string_view(csv);
        }

    private:
        const char* m_cstr;
        size_type m_size;
    };

    inline namespace cstring_view_literals
    {
        constexpr cstring_view operator""_csv(const char* str, std::size_t sz)
        {
            return cstring_view(str, sz);
        }
    }; // namespace cstring_view_literals
} // namespace util

using namespace util::cstring_view_literals;
} // namespace asbind20

#endif
