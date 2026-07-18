#ifndef ASBIND20_BIND_CALLING_CONVENTION_HPP
#define ASBIND20_BIND_CALLING_CONVENTION_HPP

#include "../detail/include_as.hpp"

namespace asbind20::detail
{
template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
struct call_conv_t
{
    constexpr operator AS_NAMESPACE_QUALIFIER asECallConvTypes() const noexcept
    {
        return CallConv;
    }
};

// Helper for specifying calling convention
template <AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
constexpr inline call_conv_t<CallConv> cc;

constexpr inline call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_GENERIC> generic_cc{};
constexpr inline call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST> cdecl_last_cc{};
} // namespace asbind20::detail

#endif
