aux_source_directory(. test_io_src)

add_executable(test_io ${test_io_src})
target_link_libraries(test_io PRIVATE shared_test_lib)
gtest_discover_tests(test_io)
