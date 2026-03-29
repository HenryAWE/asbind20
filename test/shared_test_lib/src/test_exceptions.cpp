#include <asbind_test/framework.hpp>
#include <asbind_test/test_exceptions.hpp>
#include <asbind20/detail/throw_helper.hpp>

namespace asbind_test
{
instantly_throw::instantly_throw()
    : m_placeholder{}
{
#ifndef ASBIND20_NO_EXCEPTIONS
    throw expected_ex{};
#else
    []()
    {
        GTEST_FAIL()
            << "instantly_throw::instantly_throw() called";
    }();
    std::terminate();
#endif
}

throw_on_copy::throw_on_copy(const throw_on_copy&)
{
#ifndef ASBIND20_NO_EXCEPTIONS
    throw expected_ex{};
#else
    []()
    {
        GTEST_FAIL()
            << "throw_on_copy::throw_on_copy(const throw_on_copy) called";
    }();
    std::terminate();
#endif
}

throw_on_copy& throw_on_copy::operator=(const throw_on_copy&)
{
#ifndef ASBIND20_NO_EXCEPTIONS
    throw expected_ex{};
#else
    []()
    {
        GTEST_FAIL()
            << "throw_on_copy::operator=(const throw_on_copy&) called";
    }();
    std::terminate();
#endif
}
} // namespace asbind_test
