aux_source_directory(. test_debug_src)
add_executable(test_debug ${test_debug_src})
target_link_libraries(test_debug PRIVATE shared_test_lib)
gtest_discover_tests(test_debug DISCOVERY_TIMEOUT 1000)
