#include <asbind20/asbind.hpp>
#include <cstring>

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

bool has_max_portability(const char* options)
{
    return std::strstr(options, "AS_MAX_PORTABILITY") != nullptr;
}

bool has_exceptions(const char* options)
{
    return std::strstr(options, "AS_NO_EXCEPTIONS") == nullptr;
}
} // namespace asbind20
