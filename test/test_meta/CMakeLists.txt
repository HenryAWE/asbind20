aux_source_directory(. test_meta_src)
add_executable(test_meta ${test_meta_src})
target_link_libraries(test_meta PRIVATE shared_test_lib)
gtest_discover_tests(test_meta DISCOVERY_TIMEOUT 1000)
