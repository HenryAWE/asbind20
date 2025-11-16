// Exception classes for testing exception guarantee

#pragma once

#include <exception>
#include <asbind20/asbind.hpp>

namespace asbind_test
{
class expected_ex : public std::exception
{
public:
    static constexpr char info[] = "expected exception";

    const char* what() const noexcept override
    {
        return info;
    }
};

class instantly_throw
{
public:
    instantly_throw()
        : m_placeholder{}
    {
        throw expected_ex{};
    }

    instantly_throw(const instantly_throw&) = default;

    ~instantly_throw() = default;

private:
    [[maybe_unused]]
    int m_placeholder[4];
};

class throw_on_copy
{
public:
    throw_on_copy()
        : m_placeholder{} {}

    throw_on_copy(const throw_on_copy&)
    {
        throw expected_ex{};
    }

    ~throw_on_copy() = default;

    throw_on_copy& operator=(const throw_on_copy&)
    {
        throw expected_ex{};
    }

private:
    [[maybe_unused]]
    int m_placeholder[4];
};

template <bool UseGeneric>
void register_instantly_throw(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    AS_NAMESPACE_QUALIFIER asQWORD flags =
        AS_NAMESPACE_QUALIFIER asGetTypeTraits<instantly_throw>() |
        AS_NAMESPACE_QUALIFIER asOBJ_POD |
        AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS;

    value_class<instantly_throw, UseGeneric>(
        engine, "instantly_throw", flags
    )
        .behaviours_by_traits(flags);
}

template <bool UseGeneric>
void register_throw_on_copy(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    AS_NAMESPACE_QUALIFIER asQWORD flags =
        AS_NAMESPACE_QUALIFIER asGetTypeTraits<throw_on_copy>() |
        AS_NAMESPACE_QUALIFIER asOBJ_POD |
        AS_NAMESPACE_QUALIFIER asOBJ_APP_CLASS_ALLINTS;

    value_class<throw_on_copy, UseGeneric>(
        engine, "throw_on_copy", flags
    )
        .behaviours_by_traits(flags);
}
} // namespace asbind_test
