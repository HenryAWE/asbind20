/**
 * @file options.hpp
 * @author HenryAWE
 * @brief Container options for customization
 */

#ifndef ASBIND20_CONTAINER_OPTIONS_HPP
#define ASBIND20_CONTAINER_OPTIONS_HPP

#pragma once

#include <concepts>
#include "../detail/include_as.hpp"

namespace asbind20::container
{
/**
 * @brief Concept of type information policies
 */
template <typename T>
concept typeinfo_policy = requires(AS_NAMESPACE_QUALIFIER asITypeInfo* ti) {
    typename T::typeinfo_policy_tag;
    { T::get_type_info(ti) } -> std::same_as<AS_NAMESPACE_QUALIFIER asITypeInfo*>;
    { T::get_type_id(ti) } -> std::same_as<int>;
};

/**
 * @defgroup TypeInfoPolicies Policies for how containers deal with the type information
 */
///@{

/**
 * @brief The type information itself is for the element
 */
struct typeinfo_identity
{
    using typeinfo_policy_tag = void;

    static auto get_type_info(AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
        -> AS_NAMESPACE_QUALIFIER asITypeInfo*
    {
        return ti;
    }

    static int get_type_id(AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
    {
        if(!ti) [[unlikely]]
            return AS_NAMESPACE_QUALIFIER asINVALID_ARG;
        return ti->GetTypeId();
    }
};

/**
 * @brief The subtype of type information is for the element
 *
 * @tparam Idx Index of the subtype
 */
template <AS_NAMESPACE_QUALIFIER asUINT Idx>
struct typeinfo_subtype :
    public std::integral_constant<AS_NAMESPACE_QUALIFIER asUINT, Idx>
{
    using typeinfo_policy_tag = void;

    static auto get_type_info(AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
        -> AS_NAMESPACE_QUALIFIER asITypeInfo*
    {
        if(!ti) [[unlikely]]
            return nullptr;
        return ti->GetSubType(Idx);
    }

    static int get_type_id(AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
    {
        if(!ti) [[unlikely]]
            return AS_NAMESPACE_QUALIFIER asINVALID_ARG;
        return ti->GetSubTypeId(Idx);
    }
};

/// @}
} // namespace asbind20::container

#endif
