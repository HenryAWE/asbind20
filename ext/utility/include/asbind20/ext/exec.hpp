#ifndef ASBIND20_EXT_EXEC_HPP
#define ASBIND20_EXT_EXEC_HPP

#pragma once

#include <ios>
#include <filesystem>
#include <asbind20/asbind.hpp>

namespace asbind20::ext
{
/**
 * @brief Load a string as script section
 *
 * @return int AngelScript error code
 */
int load_string(
    AS_NAMESPACE_QUALIFIER asIScriptModule* m,
    const char* section_name,
    std::string_view code,
    int line_offset = 0
);

/**
 * @brief Load a file as script section
 *
 * @return int AngelScript error code
 */
int load_file(
    AS_NAMESPACE_QUALIFIER asIScriptModule* m,
    const std::filesystem::path& filename,
    std::ios_base::openmode mode = std::ios_base::in
);

/**
 * @brief Execute a piece of AngelScript code
 */
int exec(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    std::string_view code,
    AS_NAMESPACE_QUALIFIER asIScriptContext* ctx = nullptr
);
} // namespace asbind20::ext

#endif
