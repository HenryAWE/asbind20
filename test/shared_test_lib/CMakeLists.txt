include(GoogleTest)

option(asbind_dev_warning_as_error "Treat warnings as errors in development builds" ON)
mark_as_advanced(asbind_dev_warning_as_error)

aux_source_directory(src shared_test_lib_src)
add_library(shared_test_lib STATIC ${shared_test_lib_src})
target_link_libraries(shared_test_lib PUBLIC asbind20 GTest::gtest GTest::gtest_main)
# Enable warnings for development
if(${asbind_dev_warning_as_error})
    target_compile_options(shared_test_lib PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
        # Workaround for GCC and GoogleTest, see https://github.com/google/googletest/issues/4232
        $<$<CXX_COMPILER_ID:GNU>:-Wno-error=restrict -Wno-restrict>
    )
endif()
target_include_directories(shared_test_lib PUBLIC "include")

add_subdirectory("test")
