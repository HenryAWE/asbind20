/**
 * @file debugging.hpp
 * @author HenryAWE
 * @brief Tools for debugging
 */

#ifndef ASBIND20_DEBUGGING_HPP
#define ASBIND20_DEBUGGING_HPP

#pragma once

#include <cassert>
#include <string>
#include "detail/include_as.hpp"
#include "detail/throw_helper.hpp"

/**
 * @brief Tools for debugging
 */
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
    extract_string_result(error_type e) noexcept
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

    extract_string_result(std::string str)
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
    assert(factory != nullptr);
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
    return extract_string(&factory, str);
}

#ifdef ASBIND20_HAS_GET_STRING_FACTORY

[[nodiscard]]
inline extract_string_result extract_string(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, const void* str
)
{
    assert(engine != nullptr);

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

/**
 * @brief Get script section name of function
 *
 * This helper can handle the different interfaces for getting the section name across AngelScript versions.
 *
 * @param func Script function. It cannot be nullptr.
 */
[[nodiscard]]
inline const char* get_function_section_name(
    const AS_NAMESPACE_QUALIFIER asIScriptFunction* func
)
{
    assert(func != nullptr);

#if ANGELSCRIPT_VERSION >= 23800

    const char* result;
    func->GetDeclaredAt(&result, nullptr, nullptr);
    return result;

#else // 2.37
    return func->GetScriptSectionName();
#endif
}

/**
 * @brief GC statistics
 */
struct gc_statistics
{
    using value_type = AS_NAMESPACE_QUALIFIER asUINT;

    value_type current_size;
    value_type total_destroyed;
    value_type total_detected;
    value_type new_objects;
    value_type total_new_destroyed;

    bool operator==(const gc_statistics&) const noexcept = default;
};

/**
 * @brief Get the GC statistics
 *
 * @param engine Script engine. It cannot be nullptr.
 */
[[nodiscard]]
inline gc_statistics get_gc_statistics(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    assert(engine != nullptr);

    gc_statistics result{};
    engine->GetGCStatistics(
        &result.current_size,
        &result.total_destroyed,
        &result.total_detected,
        &result.new_objects,
        &result.total_new_destroyed
    );
    return result;
}
} // namespace asbind20::debugging

#endif
