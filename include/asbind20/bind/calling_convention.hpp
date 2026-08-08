#ifndef ASBIND20_BIND_CALLING_CONVENTION_HPP
#define ASBIND20_BIND_CALLING_CONVENTION_HPP

#include "../detail/include_as.hpp"

namespace asbind20::detail
{
using call_conv_type = AS_NAMESPACE_QUALIFIER asECallConvTypes;

template <call_conv_type CallConv>
struct call_conv_t
{
    constexpr operator call_conv_type() const noexcept
    {
        return CallConv;
    }
};

// Helper for specifying calling convention
template <call_conv_type CallConv>
constexpr inline call_conv_t<CallConv> cc;

// Common conventions for generated wrappers
constexpr inline call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_GENERIC> generic_cc{};
constexpr inline call_conv_t<AS_NAMESPACE_QUALIFIER asCALL_CDECL_OBJLAST> cdecl_last_cc{};
} // namespace asbind20::detail

#endif
