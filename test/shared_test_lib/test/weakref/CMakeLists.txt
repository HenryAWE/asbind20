aux_source_directory(. test_weakref_src)

add_executable(test_weakref ${test_weakref_src})
target_link_libraries(test_weakref PRIVATE shared_test_lib)
gtest_discover_tests(test_weakref DISCOVERY_TIMEOUT 1000)
