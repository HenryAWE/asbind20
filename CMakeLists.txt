cmake_minimum_required(VERSION 3.20)

file(STRINGS "include/asbind20/asbind.hpp" asbind20_version_major_line REGEX "^#define ASBIND20_VERSION_MAJOR [0-9]+$")
file(STRINGS "include/asbind20/asbind.hpp" asbind20_version_minor_line REGEX "^#define ASBIND20_VERSION_MINOR [0-9]+$")
file(STRINGS "include/asbind20/asbind.hpp" asbind20_version_patch_line REGEX "^#define ASBIND20_VERSION_PATCH [0-9]+$")
string(REGEX REPLACE "^#define ASBIND20_VERSION_MAJOR ([0-9]+)$" "\\1" asbind20_version_major "${asbind20_version_major_line}")
string(REGEX REPLACE "^#define ASBIND20_VERSION_MINOR ([0-9]+)$" "\\1" asbind20_version_minor "${asbind20_version_minor_line}")
string(REGEX REPLACE "^#define ASBIND20_VERSION_PATCH ([0-9]+)$" "\\1" asbind20_version_patch "${asbind20_version_patch_line}")
unset(asbind20_version_major_line)
unset(asbind20_version_minor_line)
unset(asbind20_version_patch_line)

message("asbind20 v${asbind20_version_major}.${asbind20_version_minor}.${asbind20_version_patch}")

project(
    asbind20
    VERSION ${asbind20_version_major}.${asbind20_version_minor}.${asbind20_version_patch}
    LANGUAGES CXX
)

find_package(Threads REQUIRED)
find_package(Angelscript 2.37.0 CONFIG REQUIRED)

add_library(asbind20 INTERFACE)

# Add an ALIAS target for using the library by add_subdirectory()
add_library(asbind20::asbind20 ALIAS asbind20)

option(asbind_build_ext "Build extensions" ON)
option(asbind_build_doc "Build documentation" OFF)

include(GNUInstallDirs)

option(asbind_build_test "Build tests" OFF)
mark_as_advanced(asbind_build_test)
option(asbind_build_bench "Build benchmarks" OFF)
mark_as_advanced(asbind_build_bench)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

if(NOT DEFINED asbind_cxxstd)
    set(asbind_cxxstd 20 CACHE STRING "Preferred C++ standard to use for asbind20")
endif()
target_compile_features(asbind20 INTERFACE cxx_std_${asbind_cxxstd})

if(${asbind_build_test})
    message(STATUS "Fetching GoogleTest v1.17.0 from GitHub")

    include(FetchContent)

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
    )
    set(BUILD_GMOCK OFF)
    set(INSTALL_GTEST OFF)
    FetchContent_MakeAvailable(googletest)

    enable_testing()
endif()

if(${asbind_build_bench})
    message(STATUS "Fetching Google Benchmark v1.9.4 from GitHub")

    include(FetchContent)

    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.9.4
    )
    set(BENCHMARK_ENABLE_INSTALL OFF)
    set(BENCHMARK_ENABLE_TESTING OFF)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
    FetchContent_MakeAvailable(benchmark)
endif()

if(${asbind_build_ext})
    add_subdirectory(ext)
endif()

if(${asbind_build_doc})
    add_subdirectory(doc)
endif()

target_include_directories(asbind20 INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(asbind20 INTERFACE Angelscript::angelscript)

install(
    TARGETS asbind20
    EXPORT asbind20Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.inl"
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "asbind20ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/cmake/asbind20Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/asbind20Config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/cmake/asbind20
)
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/asbind20Config.cmake"
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/cmake/asbind20
)
install(
    EXPORT asbind20Targets
    FILE asbind20Targets.cmake
    NAMESPACE asbind20::
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/cmake/asbind20
)

if(${asbind_build_test})
    add_subdirectory(test)
endif()

if(${asbind_build_bench})
    add_subdirectory(bench)
endif()
