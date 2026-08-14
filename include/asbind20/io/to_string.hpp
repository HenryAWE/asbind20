/**
 * @file io/to_string.hpp
 * @author HenryAWE
 * @brief Helpers for easily converting some AngelScript library type to string
 */

#ifndef ASBIND20_IO_TO_STRING_HPP
#define ASBIND20_IO_TO_STRING_HPP

#pragma once

#include <string>
#include "../fwd.hpp"
#ifdef ASBIND20_HAS_LIB_FORMAT
#    include <format>
#endif

namespace asbind20
{
namespace detail
{
#define ASBIND20_DETAIL_TO_CSTR_HELPER(enumerator) \
case AS_NAMESPACE_QUALIFIER enumerator: return #enumerator

    constexpr const char* state_to_cstr(AS_NAMESPACE_QUALIFIER asEContextState state) noexcept
    {
        switch(state)
        {
            ASBIND20_DETAIL_TO_CSTR_HELPER(asEXECUTION_FINISHED);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asEXECUTION_SUSPENDED);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asEXECUTION_ABORTED);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asEXECUTION_EXCEPTION);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asEXECUTION_PREPARED);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asEXECUTION_UNINITIALIZED);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asEXECUTION_ACTIVE);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asEXECUTION_ERROR);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asEXECUTION_DESERIALIZATION);

        [[unlikely]] default:
            return nullptr;
        }
    }

    constexpr const char* ret_to_cstr(AS_NAMESPACE_QUALIFIER asERetCodes ret) noexcept
    {
        switch(ret)
        {
            ASBIND20_DETAIL_TO_CSTR_HELPER(asSUCCESS);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asERROR);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asCONTEXT_ACTIVE);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asCONTEXT_NOT_FINISHED);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asCONTEXT_NOT_PREPARED);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asINVALID_ARG);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asNO_FUNCTION);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asNOT_SUPPORTED);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asINVALID_NAME);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asNAME_TAKEN);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asINVALID_DECLARATION);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asINVALID_OBJECT);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asINVALID_TYPE);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asALREADY_REGISTERED);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asMULTIPLE_FUNCTIONS);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asNO_MODULE);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asNO_GLOBAL_VAR);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asINVALID_CONFIGURATION);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asINVALID_INTERFACE);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asCANT_BIND_ALL_FUNCTIONS);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asLOWER_ARRAY_DIMENSION_NOT_REGISTERED);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asWRONG_CONFIG_GROUP);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asCONFIG_GROUP_IS_IN_USE);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asILLEGAL_BEHAVIOUR_FOR_TYPE);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asWRONG_CALLING_CONV);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asBUILD_IN_PROGRESS);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asINIT_GLOBAL_VARS_FAILED);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asOUT_OF_MEMORY);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asMODULE_IS_IN_USE);

        [[unlikely]] default:
            return nullptr;
        }
    }

    constexpr const char* msg_type_to_cstr(AS_NAMESPACE_QUALIFIER asEMsgType msg_type) noexcept
    {
        switch(msg_type)
        {
            ASBIND20_DETAIL_TO_CSTR_HELPER(asMSGTYPE_ERROR);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asMSGTYPE_WARNING);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asMSGTYPE_INFORMATION);

        [[unlikely]] default:
            return nullptr;
        }
    }

    constexpr const char* tc_to_cstr(AS_NAMESPACE_QUALIFIER asETokenClass tc) noexcept
    {
        switch(tc)
        {
            ASBIND20_DETAIL_TO_CSTR_HELPER(asTC_UNKNOWN);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asTC_KEYWORD);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asTC_VALUE);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asTC_IDENTIFIER);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asTC_COMMENT);
            ASBIND20_DETAIL_TO_CSTR_HELPER(asTC_WHITESPACE);

        [[unlikely]] default:
            return nullptr;
        }
    }

#undef ASBIND20_DETAIL_TO_CSTR_HELPER
} // namespace detail

/**
 * @brief Convert context state enum to string
 *
 * @param state Context state
 * @return String representation of the state.
 *         If the state value is invalid, the result will be `"asEContextState({state})"`,
 *         e.g. `"asEContextState(-1)"`.
 */
[[nodiscard]]
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

[[nodiscard]]
inline std::wstring to_wstring(AS_NAMESPACE_QUALIFIER asEContextState state)
{
    const char* cstr = detail::state_to_cstr(state);
    if(cstr) [[likely]]
    {
        std::string_view sv = cstr;
        return std::wstring(sv.cbegin(), sv.cend());
    }
    else
    {
#ifdef ASBIND20_HAS_LIB_FORMAT
        return std::format(L"asEContextState({})", static_cast<int>(state));
#else
        return L"asEContextState(" +
               std::to_wstring(static_cast<int>(state)) +
               L')';
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
[[nodiscard]]
inline std::string to_string(AS_NAMESPACE_QUALIFIER asERetCodes ret)
{
    const char* cstr = detail::ret_to_cstr(ret);
    if(cstr) [[likely]]
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

[[nodiscard]]
inline std::wstring to_wstring(AS_NAMESPACE_QUALIFIER asERetCodes ret)
{
    const char* cstr = detail::ret_to_cstr(ret);
    if(cstr) [[likely]]
    {
        std::string_view sv = cstr;
        return std::wstring(sv.cbegin(), sv.cend());
    }
    else
    {
#ifdef ASBIND20_HAS_LIB_FORMAT
        return std::format(L"asERetCodes({})", static_cast<int>(ret));
#else
        return L"asERetCodes(" +
               std::to_wstring(static_cast<int>(ret)) +
               L')';
#endif
    }
}

[[nodiscard]]
inline std::string to_string(AS_NAMESPACE_QUALIFIER asEMsgType msg_type)
{
    const char* cstr = detail::msg_type_to_cstr(msg_type);
    if(cstr) [[likely]]
        return cstr;
    else
    {
#ifdef ASBIND20_HAS_LIB_FORMAT
        return std::format("asEMsgType({})", static_cast<int>(msg_type));
#else
        return "asEMsgType(" +
               std::to_string(static_cast<int>(msg_type)) +
               ')';
#endif
    }
}

[[nodiscard]]
inline std::wstring to_wstring(AS_NAMESPACE_QUALIFIER asEMsgType msg_type)
{
    const char* cstr = detail::msg_type_to_cstr(msg_type);
    if(cstr) [[likely]]
    {
        std::string_view sv = cstr;
        return std::wstring(sv.cbegin(), sv.cend());
    }
    else
    {
#ifdef ASBIND20_HAS_LIB_FORMAT
        return std::format(L"asEMsgType({})", static_cast<int>(msg_type));
#else
        return L"asEMsgType(" +
               std::to_wstring(static_cast<int>(msg_type)) +
               L')';
#endif
    }
}

[[nodiscard]]
inline std::string to_string(AS_NAMESPACE_QUALIFIER asETokenClass tc)
{
    const char* cstr = detail::tc_to_cstr(tc);
    if(cstr) [[likely]]
        return cstr;
    else
    {
#ifdef ASBIND20_HAS_LIB_FORMAT
        return std::format("asETokenClass({})", static_cast<int>(tc));
#else
        return "asETokenClass(" +
               std::to_string(static_cast<int>(tc)) +
               ')';
#endif
    }
}

[[nodiscard]]
inline std::wstring to_wstring(AS_NAMESPACE_QUALIFIER asETokenClass tc)
{
    const char* cstr = detail::tc_to_cstr(tc);
    if(cstr) [[likely]]
    {
        std::string_view sv = cstr;
        return std::wstring(sv.cbegin(), sv.cend());
    }
    else
    {
#ifdef ASBIND20_HAS_LIB_FORMAT
        return std::format(L"asETokenClass({})", static_cast<int>(tc));
#else
        return L"asETokenClass(" +
               std::to_wstring(static_cast<int>(tc)) +
               L')';
#endif
    }
}
} // namespace asbind20

#endif
