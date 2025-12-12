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
inline std::string extract_string(
    AS_NAMESPACE_QUALIFIER asIStringFactory* factory, const void* str
)
{
    assert(factory != nullptr);

    AS_NAMESPACE_QUALIFIER asUINT sz = 0;

    int r = factory->GetRawStringData(str, nullptr, &sz);
    if(r < 0) [[unlikely]]
    {
        ASBIND20_THROW(std::runtime_error, ("failed to get raw string length"));
    }

    std::string result;
    result.resize(sz);

    r = factory->GetRawStringData(str, result.data(), nullptr);
    if(r < 0) [[unlikely]]
    {
        ASBIND20_THROW(std::runtime_error, ("failed to get raw string data"));
    }

    return result;
}

// AngelScript 2.38+ supports retrieving string factory from engine
#if ANGELSCRIPT_VERSION >= 23800

[[nodiscard]]
inline std::string extract_string(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine, const void* str
)
{
    AS_NAMESPACE_QUALIFIER asIStringFactory* factory = nullptr;
    int r = engine->GetStringFactory(nullptr, &factory);
    if(r != AS_NAMESPACE_QUALIFIER asSUCCESS || !factory) [[unlikely]]
    {
        ASBIND20_THROW(std::runtime_error, ("failed to get string factory"));
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
