/**
 * @file io/to_string.hpp
 * @author HenryAWE
 * @brief Helpers for easily converting some AngelScript library type to string
 */

#ifndef ASBIND20_IO_TO_STRING_HPP
#define ASBIND20_IO_TO_STRING_HPP

#pragma once

#include <version>
#include <string>
#include "../detail/include_as.hpp"

#ifdef __cpp_lib_format
#    define ASBIND20_HAS_LIB_FORMAT __cpp_lib_format
#    include <format>
#endif

namespace asbind20
{
namespace detail
{
    constexpr const char* state_to_cstr(AS_NAMESPACE_QUALIFIER asEContextState state) noexcept
    {
        switch(state)
        {
        case AS_NAMESPACE_QUALIFIER asEXECUTION_FINISHED:
            return "asEXECUTION_FINISHED";
        case AS_NAMESPACE_QUALIFIER asEXECUTION_SUSPENDED:
            return "asEXECUTION_SUSPENDED";
        case AS_NAMESPACE_QUALIFIER asEXECUTION_ABORTED:
            return "asEXECUTION_ABORTED";
        case AS_NAMESPACE_QUALIFIER asEXECUTION_EXCEPTION:
            return "asEXECUTION_EXCEPTION";
        case AS_NAMESPACE_QUALIFIER asEXECUTION_PREPARED:
            return "asEXECUTION_PREPARED";
        case AS_NAMESPACE_QUALIFIER asEXECUTION_UNINITIALIZED:
            return "asEXECUTION_UNINITIALIZED";
        case AS_NAMESPACE_QUALIFIER asEXECUTION_ACTIVE:
            return "asEXECUTION_ACTIVE";
        case AS_NAMESPACE_QUALIFIER asEXECUTION_ERROR:
            return "asEXECUTION_ERROR";
        case AS_NAMESPACE_QUALIFIER asEXECUTION_DESERIALIZATION:
            return "asEXECUTION_DESERIALIZATION";

        [[unlikely]] default:
            return nullptr;
        }
    }

    constexpr const char* ret_to_cstr(AS_NAMESPACE_QUALIFIER asERetCodes ret) noexcept
    {
        switch(ret)
        {
        case AS_NAMESPACE_QUALIFIER asSUCCESS:
            return "asSUCCESS";
        case AS_NAMESPACE_QUALIFIER asERROR:
            return "asERROR";
        case AS_NAMESPACE_QUALIFIER asCONTEXT_ACTIVE:
            return "asCONTEXT_ACTIVE";
        case AS_NAMESPACE_QUALIFIER asCONTEXT_NOT_FINISHED:
            return "asCONTEXT_NOT_FINISHED";
        case AS_NAMESPACE_QUALIFIER asCONTEXT_NOT_PREPARED:
            return "asCONTEXT_NOT_PREPARED";
        case AS_NAMESPACE_QUALIFIER asINVALID_ARG:
            return "asINVALID_ARG";
        case AS_NAMESPACE_QUALIFIER asNO_FUNCTION:
            return "asNO_FUNCTION";
        case AS_NAMESPACE_QUALIFIER asNOT_SUPPORTED:
            return "asNOT_SUPPORTED";
        case AS_NAMESPACE_QUALIFIER asINVALID_NAME:
            return "asINVALID_NAME";
        case AS_NAMESPACE_QUALIFIER asNAME_TAKEN:
            return "asNAME_TAKEN";
        case AS_NAMESPACE_QUALIFIER asINVALID_DECLARATION:
            return "asINVALID_DECLARATION";
        case AS_NAMESPACE_QUALIFIER asINVALID_OBJECT:
            return "asINVALID_OBJECT";
        case AS_NAMESPACE_QUALIFIER asINVALID_TYPE:
            return "asINVALID_TYPE";
        case AS_NAMESPACE_QUALIFIER asALREADY_REGISTERED:
            return "asALREADY_REGISTERED";
        case AS_NAMESPACE_QUALIFIER asMULTIPLE_FUNCTIONS:
            return "asMULTIPLE_FUNCTIONS";
        case AS_NAMESPACE_QUALIFIER asNO_MODULE:
            return "asNO_MODULE";
        case AS_NAMESPACE_QUALIFIER asNO_GLOBAL_VAR:
            return "asNO_GLOBAL_VAR";
        case AS_NAMESPACE_QUALIFIER asINVALID_CONFIGURATION:
            return "asINVALID_CONFIGURATION";
        case AS_NAMESPACE_QUALIFIER asINVALID_INTERFACE:
            return "asINVALID_INTERFACE";
        case AS_NAMESPACE_QUALIFIER asCANT_BIND_ALL_FUNCTIONS:
            return "asCANT_BIND_ALL_FUNCTIONS";
        case AS_NAMESPACE_QUALIFIER asLOWER_ARRAY_DIMENSION_NOT_REGISTERED:
            return "asLOWER_ARRAY_DIMENSION_NOT_REGISTERED";
        case AS_NAMESPACE_QUALIFIER asWRONG_CONFIG_GROUP:
            return "asWRONG_CONFIG_GROUP";
        case AS_NAMESPACE_QUALIFIER asCONFIG_GROUP_IS_IN_USE:
            return "asCONFIG_GROUP_IS_IN_USE";
        case AS_NAMESPACE_QUALIFIER asILLEGAL_BEHAVIOUR_FOR_TYPE:
            return "asILLEGAL_BEHAVIOUR_FOR_TYPE";
        case AS_NAMESPACE_QUALIFIER asWRONG_CALLING_CONV:
            return "asWRONG_CALLING_CONV";
        case AS_NAMESPACE_QUALIFIER asBUILD_IN_PROGRESS:
            return "asBUILD_IN_PROGRESS";
        case AS_NAMESPACE_QUALIFIER asINIT_GLOBAL_VARS_FAILED:
            return "asINIT_GLOBAL_VARS_FAILED";
        case AS_NAMESPACE_QUALIFIER asOUT_OF_MEMORY:
            return "asOUT_OF_MEMORY";
        case AS_NAMESPACE_QUALIFIER asMODULE_IS_IN_USE:
            return "asMODULE_IS_IN_USE";

        [[unlikely]] default:
            return nullptr;
        }
    }
} // namespace detail

/**
 * @brief Convert context state enum to string
 *
 * @param state Context state
 * @return String representation of the state.
 *         If the state value is invalid, the result will be `"asEContextState({state})"`,
 *         e.g. `"asEContextState(-1)"`.
 */
inline std::string to_string(AS_NAMESPACE_QUALIFIER asEContextState state)
{
    const char* cstr = detail::state_to_cstr(state);
    if(cstr) [[likely]]
        return cstr;
    else
    {
#ifdef ASBIND20_HAS_LIB_FORMAT
        return std::format("asEContextState({})", static_cast<int>(state));
#else
        return "asEContextState(" +
               std::to_string(static_cast<int>(state)) +
               ')';
#endif
    }
}

/**
 * @brief Convert return code to string
 *
 * @param ret Return code
 * @return String representation of the return code.
 *         If the value is invalid, the result will be `"asERetCodes({ret})"`,
 *         e.g. `"asERetCodes(1)"`.
 */
inline std::string to_string(AS_NAMESPACE_QUALIFIER asERetCodes ret)
{
    const char* cstr = detail::ret_to_cstr(ret);
    if(cstr) [[unlikely]]
        return cstr;
    else
    {
#ifdef ASBIND20_HAS_LIB_FORMAT
        return std::format("asERetCodes({})", static_cast<int>(ret));
#else
        return "asERetCodes(" +
               std::to_string(static_cast<int>(ret)) +
               ')';
#endif
    }
}
} // namespace asbind20

#endif
