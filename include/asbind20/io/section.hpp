/**
 * @file io/section.hpp
 * @author HenryAWE
 * @brief Helpers for loading script section
 */

#ifndef ASBIND20_IO_SECTION_HPP
#define ASBIND20_IO_SECTION_HPP

#pragma once

#include <string_view>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <asbind20/asbind.hpp>

namespace asbind20::io
{
/**
 * @brief Load a string as script section
 *
 * @return AngelScript error code
 */
inline int load_string(
    module_reference m,
    cstring_ref section_name,
    std::string_view code,
    int line_offset = 0
)
{
    return m.AddScriptSection(
        section_name.c_str(),
        code.data(),
        code.size(),
        line_offset
    );
}

/**
 * @brief Load a string as script section
 *
 * @return AngelScript error code
 */
inline int load_string(
    module_pointer m,
    cstring_ref section_name,
    std::string_view code,
    int line_offset = 0
)
{
    if(!m) [[unlikely]]
        return AS_NAMESPACE_QUALIFIER asINVALID_ARG;
    return load_string(*m, section_name, code, line_offset);
}

/**
 * @brief Load a file as script section
 *
 * @return AngelScript error code
 */
inline int load_file(
    module_reference m,
    const std::filesystem::path& filename,
    std::ios_base::openmode mode = std::ios_base::in
)
{
    std::string code;
    {
        std::ifstream ifs(filename, std::ios_base::in | mode);
        if(!ifs.good())
            return AS_NAMESPACE_QUALIFIER asERROR;
        std::ostringstream ss;
        ss << ifs.rdbuf();
        code = std::move(ss).str();
    }

    // Force UTF-8 encoding
    // This can prevent some issues on Windows
    auto section_name = filename.u8string();
    return load_string(
        m,
        reinterpret_cast<const char*>(section_name.c_str()),
        code
    );
}

/**
 * @brief Load a file as script section
 *
 * @return AngelScript error code
 */
inline int load_file(
    module_pointer m,
    const std::filesystem::path& filename,
    std::ios_base::openmode mode = std::ios_base::in
)
{
    if(!m) [[unlikely]]
        return AS_NAMESPACE_QUALIFIER asINVALID_ARG;
    return load_file(*m, filename, mode);
}
} // namespace asbind20::io

#endif
