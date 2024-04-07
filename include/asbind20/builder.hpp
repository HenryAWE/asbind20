#ifndef ASBIND20_BUILDER_HPP
#define ASBIND20_BUILDER_HPP

#include <angelscript.h>
#include <filesystem>
#include <string>

namespace asbind20
{
int load_file(
    asIScriptModule* m,
    const std::filesystem::path& filename,
    std::ios_base::openmode mode = 0
);
} // namespace asbind20

#endif
