#include <gtest/gtest.h>
#include <shared_test_lib.hpp>

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
#ifdef ASBIND20_TEST_SKIP_SEQUENCE_TEST
    std::cerr << "ASBIND20_TEST_SKIP_SEQUENCE_TEST defined" << std::endl;
#endif
    return RUN_ALL_TESTS();
}
