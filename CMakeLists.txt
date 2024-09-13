cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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

add_library(asbind20 STATIC)

# Add an ALIAS target for using the library by add_subdirectory()
add_library(asbind20::asbind20 ALIAS asbind20)

file(GLOB asbind20_src "include/*.hpp" "src/*.cpp")
option(asbind_build_ext "Build extensions" ON)
option(asbind_build_ext_array "Build array extension" ON)
option(asbind_build_ext_math "Build math extension" ON)
option(asbind_build_ext_helper "Build helper extension" ON)
option(asbind_build_ext_stdstring "Build std::string extension" ON)
option(asbind_build_ext_assert "Build assert extension" ON)

if(${asbind_build_ext})
    if(${asbind_build_ext_array})
        list(APPEND asbind20_src "src/ext/array.cpp")
        target_compile_definitions(asbind20 PUBLIC ASBIND20_EXT_ARRAY=1)
    endif()
    if(${asbind_build_ext_math})
        list(APPEND asbind20_src "src/ext/math.cpp")
        target_compile_definitions(asbind20 PUBLIC ASBIND20_EXT_MATH=1)
    endif()
    if(${asbind_build_ext_helper})
        list(APPEND asbind20_src "src/ext/helper.cpp")
        target_compile_definitions(asbind20 PUBLIC ASBIND20_EXT_HELPER=1)
    endif()
    if(${asbind_build_ext_stdstring})
        list(APPEND asbind20_src "src/ext/stdstring.cpp")
        target_compile_definitions(asbind20 PUBLIC ASBIND20_EXT_STDSTRING=1)
    endif()
    if(${asbind_build_ext_assert})
        list(APPEND asbind20_src "src/ext/assert.cpp")
        target_compile_definitions(asbind20 PUBLIC ASBIND20_EXT_ASSERT=1)
    endif()
endif()

include(GNUInstallDirs)

target_sources(asbind20 PRIVATE ${asbind20_src})
target_include_directories(asbind20 PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(asbind20 PUBLIC Threads::Threads)
target_link_libraries(asbind20 PUBLIC Angelscript::angelscript)
target_compile_definitions(asbind20
    PRIVATE $<$<CONFIG:Debug>:ASBIND20_DEBUG=1>
)

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

option(asbind_build_test "Build tests" OFF)
mark_as_advanced(asbind_build_test)

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

option(asbind_build_example "Build examples" OFF)
if(${asbind_build_example})
    add_subdirectory(example)
endif()
