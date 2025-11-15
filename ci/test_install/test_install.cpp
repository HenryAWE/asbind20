#include <iostream>
#include <asbind20/asbind.hpp>

int main()
{
    std::cout
        << "test_install\n"
        << asbind20::library_version()
        << std::endl;

    auto engine = asbind20::make_script_engine();

    return 0;
}
