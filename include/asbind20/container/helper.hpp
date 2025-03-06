/**
 * @file options.hpp
 * @author HenryAWE
 * @brief Container options for customization
 */

#ifndef ASBIND20_CONTAINER_OPTIONS_HPP
#define ASBIND20_CONTAINER_OPTIONS_HPP

#pragma once

#include "../detail/include_as.hpp"
#include "../utility.hpp"

namespace asbind20::container
{
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

template <typename T>
concept typeinfo_policy = requires(AS_NAMESPACE_QUALIFIER asITypeInfo* ti) {
    typename T::typeinfo_policy_tag;
    { T::get_type_info(ti) } -> std::same_as<AS_NAMESPACE_QUALIFIER asITypeInfo*>;
    { T::get_type_id(ti) } -> std::same_as<int>;
};
} // namespace asbind20::container

#endif
