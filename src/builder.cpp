#include <asbind20/builder.hpp>
#include <fstream>
#include <sstream>

namespace asbind20
{
int load_file(
    asIScriptModule* m,
    const std::filesystem::path& filename,
    std::ios_base::openmode mode
)
{
    std::string code;
    {
        std::ifstream ifs(filename, std::ios_base::in | mode);
        if(!ifs.good())
            return asERROR;
        std::stringstream ss;
        ss << ifs.rdbuf();
        code = std::move(ss).str();
    }

    return m->AddScriptSection(
        reinterpret_cast<const char*>(filename.u8string().c_str()),
        code.c_str(),
        code.size()
    );
}
} // namespace asbind20
