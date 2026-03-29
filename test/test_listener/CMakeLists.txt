aux_source_directory(. test_listener_src)

add_executable(test_listener ${test_listener_src})
target_link_libraries(test_listener PRIVATE shared_test_lib)
gtest_discover_tests(test_listener DISCOVERY_TIMEOUT 1000)
