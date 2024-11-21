include(GoogleTest)

add_library(shared_test_lib STATIC shared_test_lib.hpp shared_test_lib.cpp)
target_link_libraries(shared_test_lib PUBLIC asbind20 GTest::gtest)
target_link_libraries(shared_test_lib PUBLIC asbind20_ext_utility)
target_link_libraries(shared_test_lib PUBLIC asbind20_ext_container)
target_include_directories(shared_test_lib PUBLIC .)

add_executable(test_invoke test_invoke.cpp)
target_link_libraries(test_invoke PRIVATE shared_test_lib)
gtest_discover_tests(test_invoke)

add_executable(test_build test_build.cpp)
target_link_libraries(test_build PRIVATE shared_test_lib)
gtest_discover_tests(test_build)
configure_file(test_build_1.as ${CMAKE_CURRENT_BINARY_DIR}/test_build_1.as COPYONLY)

add_executable(test_utility test_utility.cpp)
target_link_libraries(test_utility PRIVATE shared_test_lib)
gtest_discover_tests(test_utility)

add_subdirectory(test_bind)
