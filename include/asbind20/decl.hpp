/**
 * @file decl.hpp
 * @author HenryAWE
 * @brief Tools for generating declarations of script functions/methods
 */

#ifndef ASBIND20_DECL_HPP
#define ASBIND20_DECL_HPP

#pragma once

#include "detail/include_as.hpp"
#include "type_traits.hpp"

namespace asbind20
{
template <AS_NAMESPACE_QUALIFIER asEBehaviours Beh>
struct beh_t
{
    using value_type = AS_NAMESPACE_QUALIFIER asEBehaviours;

    [[nodiscard]]
    constexpr static value_type get() noexcept
    {
        return Beh;
    }
};

template <AS_NAMESPACE_QUALIFIER asEBehaviours Beh>
constexpr inline beh_t<Beh> beh{};

namespace decl
{
    /**
     * @brief Get declaration of behaviors with determined parameters, i.e.,
     * no factory/constructor support
     *
     * @tparam Beh Script behavior except factory and constructor
     *
     * @return Null-terminated string
     */
    template <AS_NAMESPACE_QUALIFIER asEBehaviours Beh>
    consteval const char* decl_of_beh() noexcept
    {
        static_assert(
            Beh != AS_NAMESPACE_QUALIFIER asBEHAVE_CONSTRUCT && Beh != AS_NAMESPACE_QUALIFIER asBEHAVE_FACTORY,
            "Declaration of factory/constructor cannot be generated without a parameter list"
        );

        switch(Beh)
        {
        case AS_NAMESPACE_QUALIFIER asBEHAVE_GET_WEAKREF_FLAG:
            return "int&f()";
        case AS_NAMESPACE_QUALIFIER asBEHAVE_ADDREF:
        case AS_NAMESPACE_QUALIFIER asBEHAVE_RELEASE:
        case AS_NAMESPACE_QUALIFIER asBEHAVE_SETGCFLAG:
            return "void f()";
        case AS_NAMESPACE_QUALIFIER asBEHAVE_GETREFCOUNT:
            return "int f()";
        case AS_NAMESPACE_QUALIFIER asBEHAVE_GETGCFLAG:
            return "bool f()";
        case AS_NAMESPACE_QUALIFIER asBEHAVE_ENUMREFS:
        case AS_NAMESPACE_QUALIFIER asBEHAVE_RELEASEREFS:
            return "void f(int&in)";

        default:
            return "";
        }
    }
} // namespace decl
} // namespace asbind20

#endif
