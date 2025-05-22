/**
 * @file utf8.hpp
 * @author HenryAWE
 * @brief A simple self-contained UTF-8 library for string extension
 */

#ifndef ASBIND20_EXT_UTF8_HPP
#define ASBIND20_EXT_UTF8_HPP

#pragma once

#include <cassert>
#include <stdexcept>
#include <cstdint>
#include <cstddef>
#include <string>

namespace asbind20::ext::utf8
{
/**
 * @defgroup UTF-8 UTF-8 library
 *
 * Functions for handling UTF-8 encoded string
 */
/// @{

/**
 * @brief Get the byte offset of the nth character in a UTF-8 string
 *
 * @param str The UTF-8 encoded string to search
 * @param n The character index to find
 *
 * @return Byte offset of the nth character, or -1 if out of range
 */
constexpr std::size_t u8_index(std::string_view str, std::size_t n) noexcept
{
    std::size_t i = 0;
    std::size_t count = 0;

    while(i < str.size())
    {
        if(count == n)
            return i;

        std::uint8_t ch = str[i];

        if((ch & 0b1111'1000) == 0b1111'0000)
            i += 4;
        else if((ch & 0b1111'0000) == 0b1110'0000)
            i += 3;
        else if((ch & 0b1110'0000) == 0b1100'0000)
            i += 2;
        else
            ++i;

        ++count;
    }

    return std::size_t(-1);
}

/**
 * @brief Get the byte offset of the nth character from the end of a UTF-8 string
 *
 * @param str The UTF-8 encoded string to search
 * @param n The character index from the end to find
 *
 * @return Byte offset of the nth character from end, or -1 if out of range
 */
constexpr std::size_t u8_index_r(std::string_view str, std::size_t n) noexcept
{
    if(str.empty())
        return std::size_t(-1);
    if(n == 0)
        return str.size();

    std::size_t count = 0;

    std::size_t i = str.size() - 1;

    while(true)
    {
        std::uint8_t b = str[i];
        if((b & 0b1100'0000) != 0b1000'0000)
        {
            ++count;
            if(count == n)
                return i;
        }

        if(i == 0)
            break;
        --i;
    };

    return std::size_t(-1);
}

/**
 * @brief Determine the number of bytes in a UTF-8 character from its first byte
 *
 * @param first The first byte of a UTF-8 character
 *
 * @return Number of bytes in the character (1-4)
 */
constexpr unsigned int u8_bytes(char first) noexcept
{
    if((first & 0b1111'1000) == 0b1111'0000)
        return 4;
    else if((first & 0b1111'0000) == 0b1110'0000)
        return 3;
    else if((first & 0b1110'0000) == 0b1100'0000)
        return 2;
    else
        return 1;
}

/**
 * @brief Count the number of UTF-8 characters in a string
 *
 * @param str The UTF-8 encoded string to measure
 *
 * @return Number of characters in the string
 */
constexpr std::size_t u8_strlen(std::string_view str) noexcept
{
    std::size_t i = 0;
    std::size_t result = 0;

    while(i < str.size())
    {
        std::uint8_t ch = str[i];

        if((ch & 0b1111'1000) == 0b1111'0000)
            i += 4;
        else if((ch & 0b1111'0000) == 0b1110'0000)
            i += 3;
        else if((ch & 0b1110'0000) == 0b1100'0000)
            i += 2;
        else
            ++i;

        ++result;
    }

    return result;
}

/**
 * @brief Convert a sequence of UTF-8 bytes to a Unicode code point
 *
 * @param str Pointer to the start of a UTF-8 character sequence
 *
 * @return The decoded Unicode code point
 */
inline char32_t u8_bytes_to_int(const char* str) noexcept
{
    assert(str != nullptr);

    const std::uint8_t* bytes = reinterpret_cast<const std::uint8_t*>(str);

    // ASCII (single byte)
    if(bytes[0] <= 0b0111'1111)
    {
        return bytes[0];
    }
    // 2 bytes
    else if(0b1100'0000 <= bytes[0] && bytes[0] <= 0b1101'1111)
    {
        char32_t result = U'\0';
        result |= bytes[0] & 0b0001'1111;
        result <<= 6;
        result |= bytes[1] & 0b0011'1111;

        return result;
    }
    // 3 bytes
    else if(0b1110'0000 <= bytes[0] && bytes[0] <= 0b1110'1111)
    {
        char32_t result = U'\0';
        result |= bytes[0] & 0b0000'1111;
        result <<= 6;
        result |= bytes[1] & 0b0011'1111;
        result <<= 6;
        result |= bytes[2] & 0b0011'1111;

        return result;
    }
    // 4 bytes
    else if(0b1111'0000 <= bytes[0] && bytes[0] <= 0b1111'0111)
    {
        char32_t result = U'\0';
        result |= bytes[0] & 0b0000'1111;
        result <<= 6;
        result |= bytes[1] & 0b0011'1111;
        result <<= 6;
        result |= bytes[2] & 0b0011'1111;
        result <<= 6;
        result |= bytes[3] & 0b0011'1111;

        return result;
    }

    return U'\0';
}

/**
 * @brief Convert a Unicode code point to UTF-8 bytes
 *
 * @param ch The Unicode code point to encode
 * @param buf Buffer to store the UTF-8 bytes (must have space for 4 bytes)
 *
 * @return Number of bytes written to the buffer (1-4)
 */
constexpr unsigned int u8_int_to_bytes(char32_t ch, char* buf)
{
    if(ch <= 0x7F)
    {
        buf[0] = static_cast<char>(ch);
        return 1;
    }
    else if(ch <= 0x7FF)
    {
        buf[1] = static_cast<char>((ch & 0b0011'1111) | 0b1000'0000);
        buf[0] = static_cast<char>((ch >> 6) | 0b1100'0000);
        return 2;
    }
    else if(ch <= 0xFFFF)
    {
        buf[2] = static_cast<char>((ch & 0b0011'1111) | 0b1000'0000);
        buf[1] = static_cast<char>(((ch >> 6) & 0b0011'1111) | 0b1000'0000);
        buf[0] = static_cast<char>((ch >> 12) | 0b1110'0000);
        return 3;
    }
    else if(ch <= 0x10FFFF)
    {
        buf[3] = static_cast<char>((ch & 0b0011'1111) | 0b1000'0000);
        buf[2] = static_cast<char>(((ch >> 6) & 0b0011'1111) | 0b1000'0000);
        buf[1] = static_cast<char>(((ch >> 12) & 0b0011'1111) | 0b1000'0000);
        buf[0] = static_cast<char>((ch >> 18) | 0b1111'0000);
        return 4;
    }

    return 0;
}

/**
 * @brief Extract a substring from a UTF-8 string by character position
 *
 * @param sv The UTF-8 encoded string view
 * @param pos Starting character position
 * @param n Number of characters to extract (or npos for remainder)
 *
 * @return Substring view
 */
constexpr std::string_view u8_substr(
    std::string_view sv, std::size_t pos, std::size_t n = std::string_view::npos
)
{
    std::size_t idx = u8_index(sv, pos);
    if(idx == std::size_t(-1))
        return std::string_view();
    sv = sv.substr(idx);

    if(n == std::string_view::npos)
        return sv;
    idx = u8_index(sv, n);
    return sv.substr(0, idx);
}

/**
 * @brief Extract a substring from a UTF-8 string by character position from end
 *
 * @param sv The UTF-8 encoded string view
 * @param pos Starting character position from end
 * @param n Number of characters to extract (or npos for remainder)
 *
 * @return Substring view
 */
constexpr std::string_view u8_substr_r(
    std::string_view sv, std::size_t pos, std::size_t n = std::string_view::npos
)
{
    std::size_t idx = u8_index_r(sv, pos);
    if(idx == std::size_t(-1))
        return std::string_view();
    sv = sv.substr(idx);

    if(n == std::string_view::npos)
        return sv;
    idx = u8_index(sv, n);
    return sv.substr(0, idx);
}

/**
 * @brief Remove the first n characters from a UTF-8 string
 *
 * @param str The UTF-8 encoded string view
 * @param n Number of characters to remove from start
 *
 * @return String view with prefix removed
 */
constexpr std::string_view u8_remove_prefix(std::string_view str, std::size_t n)
{
    std::size_t idx = u8_index(str, n);
    if(idx == std::size_t(-1))
        return std::string_view();

    return str.substr(idx);
}

/**
 * @brief Remove the last n characters from a UTF-8 string
 *
 * @param str The UTF-8 encoded string view
 * @param n Number of characters to remove from end
 *
 * @return String view with suffix removed
 */
constexpr std::string_view u8_remove_suffix(std::string_view str, std::size_t n)
{
    std::size_t idx = u8_index_r(str, n);
    if(idx == std::size_t(-1))
        return std::string_view();

    return str.substr(0, idx);
}

/**
 * @brief Replace a range of characters in a UTF-8 string
 *
 * @param target The string to modify
 * @param idx Starting character index
 * @param n Number of characters to replace
 * @param str Replacement string
 *
 * @throws std::out_of_range if idx is invalid
 */
inline void u8_replace_inplace(std::string& target, std::size_t idx, std::size_t n, std::string_view str)
{
    std::string_view view(target);

    std::size_t target_start = u8_index(view, idx);
    if(target_start == std::size_t(-1)) [[unlikely]]
    {
        throw std::out_of_range("out of range");
    }
    if(n == 0)
        return;
    std::size_t target_stop = u8_index(view.substr(target_start), n);

    target.replace(
        target_start,
        target_stop - target_start,
        str
    );
}

/**
 * @brief Replace a range of characters in a UTF-8 string from the end
 *
 * @param target The string to modify
 * @param idx Starting character index from end
 * @param n Number of characters to replace
 * @param str Replacement string
 *
 * @throws std::out_of_range if idx is invalid
 */
inline void u8_replace_inplace_r(std::string& target, std::size_t idx, std::size_t n, std::string_view str)
{
    std::string_view view(target);

    std::size_t target_start = u8_index_r(view, idx);
    if(target_start == std::size_t(-1)) [[unlikely]]
    {
        throw std::out_of_range("out of range");
    }
    if(n == 0)
        return;
    std::size_t target_stop = u8_index(view.substr(target_start), n);

    target.replace(
        target_start,
        target_stop - target_start,
        str
    );
}

/// @}

/**
 * @brief Bidirectional iterator for UTF-8 encoded strings
 *
 * Provides character-by-character iteration over a UTF-8 string,
 * returning Unicode code points.
 */
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

    char32_t operator*() const noexcept
    {
        if(is_end())
            return U'\0';
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

/**
 * @brief Get an iterator to the beginning of a UTF-8 string
 *
 * @param str The UTF-8 encoded string view
 *
 * @return Iterator to the first character
 */
inline const_string_iterator string_cbegin(std::string_view str)
{
    return const_string_iterator(str, 0);
}

/**
 * @brief Get an iterator to the end of a UTF-8 string
 *
 * @param str The UTF-8 encoded string view
 *
 * @return Iterator to the position after the last character
 */
inline const_string_iterator string_cend(std::string_view str)
{
    return const_string_iterator(str, str.size());
}
} // namespace asbind20::ext::utf8

#endif
