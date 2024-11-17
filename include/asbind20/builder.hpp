#ifndef ASBIND20_BUILDER_HPP
#define ASBIND20_BUILDER_HPP

#pragma once

#include <filesystem>
#include <string>
#include "detail/include_as.hpp"

namespace asbind20
{
/**
 * @brief Load a string as script section
 *
 * @return int AngelScript error code
 */
int load_string(
    asIScriptModule* m,
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
    asIScriptModule* m,
    const std::filesystem::path& filename,
    std::ios_base::openmode mode = std::ios_base::in
);
} // namespace asbind20

#endif
