#include <asbind20/bind.hpp>

namespace asbind20
{
bool has_max_portability()
{
    std::string_view options = asGetLibraryOptions();

    return options.find("AS_MAX_PORTABILITY") != options.npos;
}
} // namespace asbind20
