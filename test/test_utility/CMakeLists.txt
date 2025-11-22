aux_source_directory(. test_utility_src)
add_executable(test_utility ${test_utility_src})
target_link_libraries(test_utility PRIVATE shared_test_lib)
gtest_discover_tests(test_utility)
