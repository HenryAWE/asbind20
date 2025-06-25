#include <gtest/gtest.h>
#include <shared_test_lib.hpp>

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

    if(!asbind20::has_threads())
        std::cerr << "AS_NO_THREADS" << std::endl;

    return RUN_ALL_TESTS();
}
