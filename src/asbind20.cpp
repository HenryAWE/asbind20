#include <asbind20/asbind.hpp>

namespace asbind20
{
[[nodiscard]]
const char* library_version() noexcept
{
#ifdef ASBIND20_DEBUG
    return ASBIND20_VERSION_STRING " DEBUG";
#else
    return ASBIND20_VERSION_STRING;
#endif
}
} // namespace asbind20
