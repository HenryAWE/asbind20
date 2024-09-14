cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED)

project(asbind20_test_subdir LANGUAGES CXX)

add_subdirectory(
    ../..
    # Adding a parent directory as a subdirectory needs to specify its binary_dir
    ${PROJECT_BINARY_DIR}/asbind20_subdir
)

add_executable(asbind20_test_subdir "test_subdir.cpp")
target_link_libraries(asbind20_test_subdir PRIVATE asbind20::asbind20)
