#include <asbind_test/framework.hpp>
#include <asbind20/script_error.hpp>

TEST(ScriptError, ToSystemError)
{
    auto err = std::system_error(AS_NAMESPACE_QUALIFIER asINVALID_ARG, "testing");
    EXPECT_EQ(err.code(), std::errc::invalid_argument);
}
