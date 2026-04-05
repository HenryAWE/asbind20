/**
 *
 */

#ifndef ASBIND20_BEHAVIOUR_HPP
#define ASBIND20_BEHAVIOUR_HPP

#pragma once

#include "detail/include_as.hpp"
#include "meta/validator.hpp"

namespace asbind20
{
template <AS_NAMESPACE_QUALIFIER asEBehaviours Beh>
struct behaviour_traits;

template <>
struct behaviour_traits<AS_NAMESPACE_QUALIFIER asBEHAVE_GET_WEAKREF_FLAG>
{
    static constexpr char decl[] = "int&f()";

    using matcher = meta::signature_matcher<
        meta::validator::by_addr<AS_NAMESPACE_QUALIFIER asILockableSharedBool>>;
};

template <AS_NAMESPACE_QUALIFIER asEBehaviours Beh>
requires(
    Beh == AS_NAMESPACE_QUALIFIER asBEHAVE_ADDREF ||
    Beh == AS_NAMESPACE_QUALIFIER asBEHAVE_RELEASE ||
    Beh == AS_NAMESPACE_QUALIFIER asBEHAVE_SETGCFLAG
)
struct behaviour_traits<Beh>
{
    static constexpr char decl[] = "void f()";

    using matcher = meta::signature_matcher<
        meta::validator::void_>;
};

template <>
struct behaviour_traits<AS_NAMESPACE_QUALIFIER asBEHAVE_GETREFCOUNT>
{
    static constexpr char decl[] = "int f()";

    using matcher = meta::signature_matcher<
        meta::validator::by_value<int>>;
};

template <>
struct behaviour_traits<AS_NAMESPACE_QUALIFIER asBEHAVE_GETGCFLAG>
{
    static constexpr char decl[] = "bool f()";

    using matcher = meta::signature_matcher<
        meta::validator::by_value<bool>>;
};

template <AS_NAMESPACE_QUALIFIER asEBehaviours Beh>
requires(
    Beh == AS_NAMESPACE_QUALIFIER asBEHAVE_ENUMREFS ||
    Beh == AS_NAMESPACE_QUALIFIER asBEHAVE_RELEASEREFS
)
struct behaviour_traits<Beh>
{
    static constexpr char decl[] = "void f(int&in)";

    using matcher = meta::signature_matcher<
        meta::validator::void_,
        meta::validator::by_addr<AS_NAMESPACE_QUALIFIER asIScriptEngine>>;
};

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

template <
    AS_NAMESPACE_QUALIFIER asEBehaviours Beh,
    typename Fn,
    AS_NAMESPACE_QUALIFIER asECallConvTypes CallConv>
consteval bool match_behaviour_sig(asbind20::detail::call_conv_t<CallConv>)
{
    using matcher = typename behaviour_traits<Beh>::matcher;

    return matcher{}(std::in_place_type<Fn>, asbind20::detail::cc<CallConv>);
}

template <
    AS_NAMESPACE_QUALIFIER asEBehaviours Beh,
    typename Fn>
consteval bool match_behaviour_sig()
{
    using matcher = typename behaviour_traits<Beh>::matcher;

    return matcher{}(std::in_place_type<Fn>);
}
} // namespace asbind20

#endif
