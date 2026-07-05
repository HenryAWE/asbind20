/**
 * @file detail/fwd.hpp
 * @author HenryAWE
 * @brief Forward declarations
 */

#ifndef ASBIND20_DETAIL_FWD_HPP
#define ASBIND20_DETAIL_FWD_HPP

#pragma once

#include "include_as.hpp"

namespace asbind20
{
inline namespace script_type
{
    using engine_pointer = AS_NAMESPACE_QUALIFIER asIScriptEngine*;
    using const_engine_pointer = AS_NAMESPACE_QUALIFIER asIScriptEngine const*;

    using generic_function = AS_NAMESPACE_QUALIFIER asGENFUNC_t;
    using generic_pointer = AS_NAMESPACE_QUALIFIER asIScriptGeneric*;
    using const_generic_pointer = AS_NAMESPACE_QUALIFIER asIScriptGeneric const*;

    using typeinfo_pointer = AS_NAMESPACE_QUALIFIER asITypeInfo*;
    using const_typeinfo_pointer = AS_NAMESPACE_QUALIFIER asITypeInfo const*;
} // namespace script_types
} // namespace asbind20

#endif
