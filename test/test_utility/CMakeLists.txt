file(GLOB_RECURSE test_utility_src "*.cpp")

add_executable(test_utility ${test_utility_src})
target_link_libraries(test_utility PRIVATE shared_test_lib)
gtest_discover_tests(test_utility DISCOVERY_TIMEOUT 1000)
