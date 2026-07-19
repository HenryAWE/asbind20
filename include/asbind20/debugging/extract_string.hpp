/**
 * @file debugging/extract_string.hpp
 * @author HenryAWE
 * @brief Tools for extracting string from script without knowing its underlying type
 */

#ifndef ASBIND20_DEBUGGING_EXTRACT_STRING_HPP
#define ASBIND20_DEBUGGING_EXTRACT_STRING_HPP

#pragma once

#include <new>
#include <string>
#include "../detail/config.hpp"
#include "../fwd.hpp"
#include "../detail/err_handler.hpp"

namespace asbind20::debugging
{
class extract_string_result;

extract_string_result extract_string(
    const_string_factory_pointer factory, const void* str
);

#ifdef ASBIND20_HAS_GET_STRING_FACTORY
extract_string_result extract_string(
    const_engine_pointer engine, const void* str
);

#endif

class bad_extract_string_result_access : public std::exception
{
public:
    using error_code_type = AS_NAMESPACE_QUALIFIER asERetCodes;

    bad_extract_string_result_access(const bad_extract_string_result_access&) = default;

    bad_extract_string_result_access(error_code_type r) noexcept
        : m_r(r) {}

    bad_extract_string_result_access& operator=(
        const bad_extract_string_result_access&
    ) noexcept = default;

    const char* what() const noexcept override
    {
        return "bad extract string result access";
    }

    [[nodiscard]]
    error_code_type error() const noexcept
    {
        return m_r;
    }

private:
    error_code_type m_r;
};

/**
 * @brief Result of string extraction
 */
class extract_string_result
{
    friend extract_string_result extract_string(
        const_string_factory_pointer factory, const void* str
    );

#ifdef ASBIND20_HAS_GET_STRING_FACTORY
    friend extract_string_result extract_string(
        const_engine_pointer engine, const void* str
    );

#endif

public:
    using error_type = AS_NAMESPACE_QUALIFIER asERetCodes;
    using value_type = std::string;

    extract_string_result() = delete;

private:
    explicit extract_string_result(error_type e) noexcept
        : m_error(e)
    {
        ASBIND20_ASSERT(!has_value());
    }

public:
    extract_string_result(const extract_string_result& other)
        : m_error(other.m_error)
    {
        if(other.has_value())
            new(&m_value) value_type(other.m_value);
    }

    extract_string_result(extract_string_result&& other) noexcept
        : m_error(other.m_error)
    {
        if(other.has_value())
        {
            new(&m_value) value_type(std::move(other.m_value));
            other.m_error = AS_NAMESPACE_QUALIFIER asERROR;
            std::destroy_at(&other.m_value);
        }
    }

    extract_string_result& operator=(extract_string_result&& other) noexcept
    {
        extract_string_result tmp(std::move(other));
        swap(tmp);
        return *this;
    }

    ~extract_string_result()
    {
        if(has_value())
            std::destroy_at(&m_value);
    }

    void swap(extract_string_result& other) noexcept
    {
        if(this == &other)
            return;

        using std::swap;
        if(has_value() && other.has_value())
        {
            swap(m_value, other.m_value);
            swap(m_error, other.m_error);
        }
        else if(has_value())
        {
            new(&other.m_value) value_type(std::move(m_value));
            std::destroy_at(&m_value);
            swap(m_error, other.m_error);
        }
        else if(other.has_value())
        {
            other.swap(*this);
        }
        else
        {
            swap(m_error, other.m_error);
        }
    }

    explicit extract_string_result(std::string str)
        : m_error(AS_NAMESPACE_QUALIFIER asSUCCESS)
    {
        new(&m_value) std::string(std::move(str));
    }

    [[nodiscard]]
    bool has_value() const noexcept
    {
        return static_cast<int>(m_error) >= 0;
    }

    [[nodiscard]]
    error_type error() const noexcept
    {
        return m_error;
    }

    value_type& operator*() noexcept
    {
        ASBIND20_ASSERT(has_value());
        return m_value;
    }

    const value_type& operator*() const noexcept
    {
        ASBIND20_ASSERT(has_value());
        return m_value;
    }

    [[nodiscard]]
    value_type& value()
    {
        if(!has_value())
            throw_bad_access();
        return **this;
    }

    [[nodiscard]]
    const value_type& value() const
    {
        if(!has_value())
            throw_bad_access();
        return **this;
    }

private:
    union
    {
        value_type m_value;
    };

    error_type m_error;

    [[noreturn]]
    void throw_bad_access() const
    {
        asbind20::detail::throw_<bad_extract_string_result_access>(m_error);
    }
};

inline void swap(extract_string_result& lhs, extract_string_result& rhs) noexcept
{
    lhs.swap(rhs);
}

/**
 * @brief Extracts the contents from a script string without knowing the underlying type
 *
 * @param factory The string factory
 * @param str The pointer to script string
 *
 * @throws std::runtime_error Failed to get the string
 *
 * @return std::string Result string
 */
[[nodiscard]]
inline extract_string_result extract_string(
    const_string_factory_pointer factory, const void* str
)
{
    if(!factory) [[unlikely]]
        return extract_string_result(AS_NAMESPACE_QUALIFIER asINVALID_ARG);

    std::string result;
    AS_NAMESPACE_QUALIFIER asUINT sz = 0;

    int r = factory->GetRawStringData(str, nullptr, &sz);
    if(r < 0) [[unlikely]]
        goto bad_result;

    result.resize(sz);

    r = factory->GetRawStringData(str, result.data(), nullptr);
    if(r < 0) [[unlikely]]
        goto bad_result;

    return extract_string_result(
        std::move(result)
    );

bad_result:
    return extract_string_result(
        static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(r)
    );
}

[[nodiscard]]
inline extract_string_result extract_string(
    const_string_factory_reference factory, const void* str
)
{
    return extract_string(std::addressof(factory), str);
}

#ifdef ASBIND20_HAS_GET_STRING_FACTORY

[[nodiscard]]
inline extract_string_result extract_string(
    const_engine_pointer engine, const void* str
)
{
    if(!engine) [[unlikely]]
        return extract_string_result(AS_NAMESPACE_QUALIFIER asINVALID_ARG);

    string_factory_pointer factory;
    int r = engine->GetStringFactory(nullptr, &factory);
    if(r < 0) [[unlikely]]
    {
        return extract_string_result(
            static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(r)
        );
    }

    return extract_string(factory, str);
}

#endif
} // namespace asbind20::debugging

#endif
