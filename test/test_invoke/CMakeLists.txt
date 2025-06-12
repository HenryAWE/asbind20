aux_source_directory(. test_invoke_src)

add_executable(test_invoke ${test_invoke_src})
target_link_libraries(test_invoke PRIVATE shared_test_lib)
gtest_discover_tests(test_invoke)
