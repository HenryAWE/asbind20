/**
 * @file debugging/extract_string.hpp
 * @author HenryAWE
 * @brief Tools for extracting string from script without knowing its underlying type
 */

#ifndef ASBIND20_DEBUGGING_EXTRACT_STRING_HPP
#define ASBIND20_DEBUGGING_EXTRACT_STRING_HPP

#pragma once

#include <cassert>
#include <string>
#include "../detail/include_as.hpp"
#include "../detail/throw_helper.hpp"

namespace asbind20::debugging
{
// AngelScript 2.38+ supports retrieving string factory from engine
// asIScriptEngine::GetStringFactory
#if ANGELSCRIPT_VERSION >= 23800
#    define ASBIND20_HAS_GET_STRING_FACTORY
#endif

class extract_string_result;

extract_string_result extract_string(
    AS_NAMESPACE_QUALIFIER asIStringFactory* factory, const void* str
);

#ifdef ASBIND20_HAS_GET_STRING_FACTORY
extract_string_result extract_string(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, const void* str
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
        AS_NAMESPACE_QUALIFIER asIStringFactory* factory, const void* str
    );

#ifdef ASBIND20_HAS_GET_STRING_FACTORY
    friend extract_string_result extract_string(
        AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, const void* str
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
        assert(!has_value());
    }

public:
    extract_string_result(const extract_string_result& other)
        : m_error(other.m_error)
    {
        if(other.has_value())
            new(m_data) value_type(*other);
    }

    extract_string_result(extract_string_result&& other) noexcept
        : m_error(other.m_error)
    {
        if(other.has_value())
            new(m_data) value_type(std::move(*other));
    }

    explicit extract_string_result(std::string str)
        : m_error(AS_NAMESPACE_QUALIFIER asSUCCESS)
    {
        new(m_data) std::string(std::move(str));
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
        assert(has_value());
        return *ptr();
    }

    const value_type& operator*() const noexcept
    {
        assert(has_value());
        return *ptr();
    }

    value_type& value()
    {
        if(!has_value())
            throw_bad_access();
        return **this;
    }

    const value_type& value() const
    {
        if(!has_value())
            throw_bad_access();
        return **this;
    }

private:
    alignas(value_type) std::byte m_data[sizeof(value_type)];
    error_type m_error;

    [[noreturn]]
    void throw_bad_access() const
    {
        ASBIND20_THROW(bad_extract_string_result_access, (m_error));
    }

    value_type* ptr() noexcept
    {
        return reinterpret_cast<value_type*>(m_data);
    }

    const value_type* ptr() const noexcept
    {
        return reinterpret_cast<const value_type*>(m_data);
    }
};

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
    AS_NAMESPACE_QUALIFIER asIStringFactory* factory, const void* str
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
    AS_NAMESPACE_QUALIFIER asIStringFactory& factory, const void* str
)
{
    return extract_string(std::addressof(factory), str);
}

#ifdef ASBIND20_HAS_GET_STRING_FACTORY

[[nodiscard]]
inline extract_string_result extract_string(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, const void* str
)
{
    if(!engine) [[unlikely]]
        return extract_string_result(AS_NAMESPACE_QUALIFIER asINVALID_ARG);

    AS_NAMESPACE_QUALIFIER asIStringFactory* factory;
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
