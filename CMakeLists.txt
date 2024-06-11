cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(
    asbind20
    VERSION 0.1.0
    LANGUAGES CXX
)

find_package(Angelscript CONFIG REQUIRED)

file(GLOB_RECURSE asbind20_src "include/*.hpp" "src/*.cpp")
add_library(asbind20 ${asbind20_src})
target_include_directories(asbind20 PUBLIC include)
target_link_libraries(asbind20 PUBLIC Angelscript::angelscript)

option("asbind_build_test" OFF)
if(${asbind_build_test})
    message(STATUS "Fetching GoogleTest v1.14.0 from GitHub")

    include(FetchContent)

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(BUILD_GMOCK OFF)
    set(INSTALL_GTEST OFF)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_subdirectory(test)
endif()
