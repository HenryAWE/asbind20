#include <asbind20/ext/debugging.hpp>
#include <stdexcept>

namespace asbind20::ext
{
std::string extract_string(
    AS_NAMESPACE_QUALIFIER asIStringFactory* factory, const void* str
)
{
    assert(factory);

    AS_NAMESPACE_QUALIFIER asUINT sz = 0;

    int r = factory->GetRawStringData(str, nullptr, &sz);
    if(r < 0) [[unlikely]]
    {
        throw std::runtime_error("failed to get raw string length");
    }

    std::string result;
    result.resize(sz);

    r = factory->GetRawStringData(str, result.data(), nullptr);
    if(r < 0) [[unlikely]]
    {
        throw std::runtime_error("failed to get raw string data");
    }

    return result;
}
} // namespace asbind20::ext
