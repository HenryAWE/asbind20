aux_source_directory(. test_memory_src)

add_executable(test_memory ${test_memory_src})
target_link_libraries(test_memory PRIVATE shared_test_lib)
gtest_discover_tests(test_memory DISCOVERY_TIMEOUT 1000)
