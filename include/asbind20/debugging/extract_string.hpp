/**
 * @file debugging/extract_string.hpp
 * @author HenryAWE
 * @brief Tools for extracting string from script without knowing its underlying type
 */

#ifndef ASBIND20_DEBUGGING_EXTRACT_STRING_HPP
#define ASBIND20_DEBUGGING_EXTRACT_STRING_HPP

#pragma once

#include <string>
#include "../detail/config.hpp"
#include "../fwd.hpp"
#include "../util/script_result.hpp"

namespace asbind20::debugging
{
using extract_string_result = script_result<std::string>;

/**
 * @brief Extracts the contents from a script string without knowing the underlying type
 *
 * @param factory The string factory
 * @param str The pointer to script string
 */
[[nodiscard]]
inline extract_string_result extract_string(
    const_string_factory_reference factory, const void* str
)
{
    std::string result;
    AS_NAMESPACE_QUALIFIER asUINT sz = 0;

    int r = factory.GetRawStringData(str, nullptr, &sz);
    if(r < 0) [[unlikely]]
        goto bad_result;

    result.resize(sz);
    r = factory.GetRawStringData(str, result.data(), nullptr);
    if(r < 0) [[unlikely]]
        goto bad_result;

    return extract_string_result(
        std::move(result), r
    );

bad_result:
    return {
        bad_script_result,
        static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(r)
    };
}

[[nodiscard]]
inline extract_string_result extract_string(
    const_string_factory_pointer factory, const void* str
)
{
    if(!factory) [[unlikely]]
        return {bad_script_result, AS_NAMESPACE_QUALIFIER asINVALID_ARG};

    return extract_string(*factory, str);
}

#ifdef ASBIND20_HAS_GET_STRING_FACTORY

[[nodiscard]]
inline extract_string_result extract_string(
    const_engine_reference engine, const void* str
)
{
    string_factory_pointer factory;
    if(int r = engine.GetStringFactory(nullptr, &factory); r < 0) [[unlikely]]
        return {bad_script_result, r};

    ASBIND20_ASSUME(factory != nullptr);
    return extract_string(factory, str);
}

[[nodiscard]]
inline extract_string_result extract_string(
    const_engine_pointer engine, const void* str
)
{
    if(!engine) [[unlikely]]
        return {bad_script_result, AS_NAMESPACE_QUALIFIER asINVALID_ARG};

    return extract_string(*engine, str);
}

#endif
} // namespace asbind20::debugging

#endif
