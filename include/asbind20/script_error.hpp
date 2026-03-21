#ifndef ASBIND20_SCRIPT_ERROR_HPP
#define ASBIND20_SCRIPT_ERROR_HPP

#pragma once

#include <system_error>
#include "io/to_string.hpp"

namespace asbind20
{
class asERetCodes_category : public std::error_category
{
public:
    const char* name() const noexcept override
    {
        return "asERetCodes";
    }

    std::string message(int val) const override
    {
        return to_string(static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(val));
    }

    std::error_condition default_error_condition(int val) const noexcept override
    {
        using std::errc;
        switch(static_cast<AS_NAMESPACE_QUALIFIER asERetCodes>(val))
        {
        case AS_NAMESPACE_QUALIFIER asINVALID_ARG: return errc::invalid_argument;
        case AS_NAMESPACE_QUALIFIER asOUT_OF_MEMORY: return errc::not_enough_memory;

        default:
            return std::error_condition(val, *this);
        }
    }

    static asERetCodes_category& instance()
    {
        static asERetCodes_category c;
        return c;
    }
};


} // namespace asbind20

template <>
struct std::is_error_code_enum<AS_NAMESPACE_QUALIFIER asERetCodes> :
    std::true_type
{};

// Put it in global namespace so it can be found by std::system_error
inline std::error_code make_error_code(AS_NAMESPACE_QUALIFIER asERetCodes val)
{
    return std::error_code(static_cast<int>(val), asbind20::asERetCodes_category::instance());
}

#endif
