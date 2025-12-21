#ifndef ASBIND20_DETAIL_COMPAT_HPP
#define ASBIND20_DETAIL_COMPAT_HPP

#pragma once

#include "config.hpp" // IWYU pragma: exports

namespace asbind20::compat
{
#ifndef ASBIND20_HAS_ENUM_UNDERLYING_TYPE
using script_enum_value_type = int;

#else // AngelScript <= 2.38
using script_enum_value_type = AS_NAMESPACE_QUALIFIER asINT64;

#endif
} // namespace asbind20::compat

#endif
