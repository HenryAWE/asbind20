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
#include "behaviour.hpp"

namespace asbind20
{
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

        return behaviour_traits<Beh>::decl;
    }
} // namespace decl
} // namespace asbind20

#endif
