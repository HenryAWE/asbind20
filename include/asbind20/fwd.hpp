/**
 * @file fwd.hpp
 * @author HenryAWE
 * @brief Forward declarations
 */

#ifndef ASBIND20_DETAIL_FWD_HPP
#define ASBIND20_DETAIL_FWD_HPP

#pragma once

#include "detail/include_as.hpp"

namespace asbind20
{
inline namespace script_type
{
    // Principal interfaces
    using engine_pointer = AS_NAMESPACE_QUALIFIER asIScriptEngine*;
    using const_engine_pointer = AS_NAMESPACE_QUALIFIER asIScriptEngine const*;
    using module_pointer = AS_NAMESPACE_QUALIFIER asIScriptModule*;
    using const_module_pointer = AS_NAMESPACE_QUALIFIER asIScriptModule const*;
    using context_pointer = AS_NAMESPACE_QUALIFIER asIScriptContext*;
    using const_context_pointer = AS_NAMESPACE_QUALIFIER asIScriptContext const*;

    // Secondary interfaces
    using generic_pointer = AS_NAMESPACE_QUALIFIER asIScriptGeneric*;
    using const_generic_pointer = AS_NAMESPACE_QUALIFIER asIScriptGeneric const*;
    using object_pointer = AS_NAMESPACE_QUALIFIER asIScriptObject*;
    using const_object_pointer = AS_NAMESPACE_QUALIFIER asIScriptObject const*;
    using typeinfo_pointer = AS_NAMESPACE_QUALIFIER asITypeInfo*;
    using const_typeinfo_pointer = AS_NAMESPACE_QUALIFIER asITypeInfo const*;
    using function_pointer = AS_NAMESPACE_QUALIFIER asIScriptFunction*;
    using const_function_pointer = AS_NAMESPACE_QUALIFIER asIScriptFunction const*;

    using generic_function = AS_NAMESPACE_QUALIFIER asGENFUNC_t;
} // namespace script_type

template <typename T>
class script_function_ref;
template <typename T>
class script_method_ref;
template <typename T>
class script_function;
template <typename T>
class script_method;
} // namespace asbind20

#endif
