#include <gtest/gtest.h>
#include <asbind_test/framework.hpp>

static void output_info(std::ostream& os)
{
    if(!asbind20::has_threads())
        os << "AS_NO_THREADS" << std::endl;
}

int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);

    // Output information so we can read them when CMake is configuring tests
    output_info(std::cerr);

    return RUN_ALL_TESTS();
}
