cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED)

project(asbind20_test_install LANGUAGES CXX)

find_package(asbind20 REQUIRED)

add_executable(asbind20_test_install "test_install.cpp")
target_link_libraries(asbind20_test_install PRIVATE asbind20::asbind20)
