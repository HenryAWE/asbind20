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

const char* library_options() noexcept
{
    const char* str = " "

    // Extensions
#ifdef ASBIND20_EXT_ARRAY
                      "ASBIND20_EXT_ARRAY "
#endif
#ifdef ASBIND20_EXT_STDSTRING
                      "ASBIND20_EXT_STDSTRING "
#endif
#ifdef ASBIND20_EXT_MATH
                      "ASBIND20_EXT_MATH "
#endif
#ifdef ASBIND20_EXT_ASSERT
                      "ASBIND20_EXT_ASSERT "
#endif
#ifdef ASBIND20_EXT_HELPER
                      "ASBIND20_EXT_HELPER "
#endif
        ;

    return str;
}
} // namespace asbind20
