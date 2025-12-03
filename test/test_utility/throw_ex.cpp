#include <asbind_test/framework.hpp>

#ifdef ASBIND20_NO_EXCEPTIONS

TEST(ThrowHelperDeathTest, Terminate)
{
    ASSERT_DEATH_IF_SUPPORTED(
        asbind20::detail::throw_<std::runtime_error>(""),
        ""
    ) << "Exception is disabled. Terminating...";
}

#else

TEST(ThrowHelper, Catch)
{
    EXPECT_THROW(
        asbind20::detail::throw_<std::runtime_error>(""),
        std::runtime_error
    );
}

#endif
