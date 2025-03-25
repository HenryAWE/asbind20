aux_source_directory(. test_operators_src)

add_executable(test_operators ${test_operators_src})
target_link_libraries(test_operators PRIVATE shared_test_lib)
gtest_discover_tests(test_operators)
