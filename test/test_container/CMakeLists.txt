aux_source_directory(. test_container_src)

add_executable(test_container ${test_container_src})
target_link_libraries(test_container PRIVATE shared_test_lib)
gtest_discover_tests(test_container)
