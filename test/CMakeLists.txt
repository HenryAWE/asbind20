include(GoogleTest)

add_library(shared_test_lib STATIC shared_test_lib.hpp shared_test_lib.cpp)
target_link_libraries(shared_test_lib PUBLIC asbind20 GTest::gtest)
target_link_libraries(shared_test_lib PUBLIC asbind20_ext_utility)
target_link_libraries(shared_test_lib PUBLIC asbind20_ext_container)
# Enable warnings for development
target_compile_options(shared_test_lib PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
    # Workaround for GCC and GoogleTest, see https://github.com/google/googletest/issues/4232
    $<$<CXX_COMPILER_ID:GNU>:-Wno-error=restrict -Wno-restrict>
)
target_include_directories(shared_test_lib PUBLIC .)

add_executable(test_invoke test_invoke.cpp)
target_link_libraries(test_invoke PRIVATE shared_test_lib)
gtest_discover_tests(test_invoke)

add_executable(test_utility test_utility.cpp)
target_link_libraries(test_utility PRIVATE shared_test_lib)
gtest_discover_tests(test_utility)

add_executable(test_io test_io.cpp)
target_link_libraries(test_io PRIVATE shared_test_lib)
gtest_discover_tests(test_io)

add_subdirectory(test_bind)

add_subdirectory(test_container)

add_subdirectory(test_gc)

add_subdirectory(test_concurrent)
