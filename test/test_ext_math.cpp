#include <cmath>
#include <gtest/gtest.h>
#include "asbind20/ext/math.hpp"
#include "shared.hpp"
#include <numbers>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/stdstring.hpp>

using namespace asbind_test;

TEST(ext_math, close_to)
{
    using namespace asbind20;

    EXPECT_TRUE(ext::close_to<float>(
        std::numbers::pi_v<float>, 3.14f, 0.01f
    ));
}

TEST_F(asbind_test_suite, ext_math)
{
    run_file("script/test_math.as");
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
