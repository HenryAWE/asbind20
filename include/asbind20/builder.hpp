#ifndef ASBIND20_BUILDER_HPP
#define ASBIND20_BUILDER_HPP

#pragma once

#include <angelscript.h>
#include <filesystem>
#include <string>

namespace asbind20
{
int load_string(
    asIScriptModule* m,
    const char* section_name,
    std::string_view code,
    int line_offset = 0
);

int load_file(
    asIScriptModule* m,
    const std::filesystem::path& filename,
    std::ios_base::openmode mode = std::ios_base::in
);
} // namespace asbind20

#endif
