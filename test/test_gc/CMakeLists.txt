aux_source_directory(. test_gc_src)

add_executable(test_gc ${test_gc_src})
target_link_libraries(test_gc PRIVATE shared_test_lib)
gtest_discover_tests(test_gc)
