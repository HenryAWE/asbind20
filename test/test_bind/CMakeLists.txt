file(GLOB_RECURSE test_bind_src "*.cpp")

add_executable(test_bind ${test_bind_src})
target_link_libraries(test_bind PRIVATE shared_test_lib)
gtest_discover_tests(test_bind DISCOVERY_TIMEOUT 1000)
