#include <asbind20/asbind.hpp>

namespace asbind20
{
const char* library_version() noexcept
{
#ifdef ASBIND20_DEBUG
    return ASBIND20_VERSION_STRING " DEBUG";
#else
    return ASBIND20_VERSION_STRING;
#endif
}

bool has_max_portability()
{
    std::string_view options = asGetLibraryOptions();

    return options.find("AS_MAX_PORTABILITY") != options.npos;
}
} // namespace asbind20
