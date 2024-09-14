#include <iostream>
#include <asbind20/asbind.hpp>

int main()
{
    std::cout
        << "test_subdir\n"
        << asbind20::library_version()
        << '\n'
        << asbind20::library_options()
        << std::endl;

    return 0;
}
