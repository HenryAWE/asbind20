#include <gtest/gtest.h>
#include "shared.hpp"
#include <asbind20/asbind.hpp>
#include <asbind20/ext/stdstring.hpp>

using asbind_test::asbind_test_suite;

TEST_F(asbind_test_suite, ext_stdstring)
{
    run_file("script/test_string.as");
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
