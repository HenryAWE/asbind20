aux_source_directory(. test_bind_src)

add_executable(test_bind ${test_bind_src})
target_link_libraries(test_bind PRIVATE shared_test_lib)
gtest_discover_tests(test_bind)
