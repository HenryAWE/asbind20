#include <asbind20/ext/exec.hpp>
#include <fstream>
#include <sstream>

namespace asbind20::ext
{
int load_string(
    AS_NAMESPACE_QUALIFIER asIScriptModule* m,
    const char* section_name,
    std::string_view code,
    int line_offset
)
{
    assert(m != nullptr);

    return m->AddScriptSection(
        section_name,
        code.data(),
        code.size(),
        line_offset
    );
}

int load_file(
    AS_NAMESPACE_QUALIFIER asIScriptModule* m,
    const std::filesystem::path& filename,
    std::ios_base::openmode mode
)
{
    assert(m != nullptr);

    std::string code;
    {
        std::ifstream ifs(filename, std::ios_base::in | mode);
        if(!ifs.good())
            return AS_NAMESPACE_QUALIFIER asERROR;
        std::stringstream ss;
        ss << ifs.rdbuf();
        code = std::move(ss).str();
    }

    return load_string(
        m,
        reinterpret_cast<const char*>(filename.u8string().c_str()),
        code
    );
}
} // namespace asbind20::ext
