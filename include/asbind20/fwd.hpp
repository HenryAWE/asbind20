/**
 * @file fwd.hpp
 * @author HenryAWE
 * @brief Forward declarations
 */

#ifndef ASBIND20_DETAIL_FWD_HPP
#define ASBIND20_DETAIL_FWD_HPP

#pragma once

#include "detail/include_as.hpp"
#include "detail/config.hpp"

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

    using engine_reference = AS_NAMESPACE_QUALIFIER asIScriptEngine&;
    using const_engine_reference = AS_NAMESPACE_QUALIFIER asIScriptEngine const&;
    using module_reference = AS_NAMESPACE_QUALIFIER asIScriptModule&;
    using const_module_reference = AS_NAMESPACE_QUALIFIER asIScriptModule const&;
    using context_reference = AS_NAMESPACE_QUALIFIER asIScriptContext&;
    using const_context_reference = AS_NAMESPACE_QUALIFIER asIScriptContext const&;

    // Secondary interfaces
    using generic_pointer = AS_NAMESPACE_QUALIFIER asIScriptGeneric*;
    using const_generic_pointer = AS_NAMESPACE_QUALIFIER asIScriptGeneric const*;
    using string_factory_pointer = AS_NAMESPACE_QUALIFIER asIStringFactory*;
    using const_string_factory_pointer = AS_NAMESPACE_QUALIFIER asIStringFactory const*;
    using object_pointer = AS_NAMESPACE_QUALIFIER asIScriptObject*;
    using const_object_pointer = AS_NAMESPACE_QUALIFIER asIScriptObject const*;
    using typeinfo_pointer = AS_NAMESPACE_QUALIFIER asITypeInfo*;
    using const_typeinfo_pointer = AS_NAMESPACE_QUALIFIER asITypeInfo const*;
    using function_pointer = AS_NAMESPACE_QUALIFIER asIScriptFunction*;
    using const_function_pointer = AS_NAMESPACE_QUALIFIER asIScriptFunction const*;

    using generic_reference = AS_NAMESPACE_QUALIFIER asIScriptGeneric&;
    using const_generic_reference = AS_NAMESPACE_QUALIFIER asIScriptGeneric const&;
    using string_factory_reference = AS_NAMESPACE_QUALIFIER asIStringFactory&;
    using const_string_factory_reference = AS_NAMESPACE_QUALIFIER asIStringFactory const&;
    using object_reference = AS_NAMESPACE_QUALIFIER asIScriptObject&;
    using const_object_reference = AS_NAMESPACE_QUALIFIER asIScriptObject const&;
    using typeinfo_reference = AS_NAMESPACE_QUALIFIER asITypeInfo&;
    using const_typeinfo_reference = AS_NAMESPACE_QUALIFIER asITypeInfo const&;
    using function_reference = AS_NAMESPACE_QUALIFIER asIScriptFunction&;
    using const_function_reference = AS_NAMESPACE_QUALIFIER asIScriptFunction const&;

    // Utility aliases
    using generic_function = AS_NAMESPACE_QUALIFIER asGENFUNC_t;
    using arg_index_type = AS_NAMESPACE_QUALIFIER asUINT;
} // namespace script_type

template <typename T>
class script_function_ref;
template <typename T>
class script_method_ref;
template <typename T>
class script_function;
template <typename T>
class script_method;

namespace compat
{
#ifndef ASBIND20_HAS_SCRIPT_ENUM_UNDERLYING_TYPE
    using script_enum_value_type = int;

#else // AngelScript >= 2.39
    using script_enum_value_type = AS_NAMESPACE_QUALIFIER asINT64;

#endif
} // namespace compat
} // namespace asbind20

#endif
