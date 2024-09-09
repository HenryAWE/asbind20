#include <gtest/gtest.h>
#include "shared.hpp"
#include <asbind20/asbind.hpp>
#include <asbind20/ext/array.hpp>

using namespace asbind_test;

TEST_F(asbind_test_suite, ext_array)
{
    run_file("script/test_array.as");
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
