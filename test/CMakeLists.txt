add_subdirectory(shared_test_lib)

add_executable(test_utility test_utility.cpp)
target_link_libraries(test_utility PRIVATE shared_test_lib)
gtest_discover_tests(test_utility)

add_executable(test_io test_io.cpp)
target_link_libraries(test_io PRIVATE shared_test_lib)
gtest_discover_tests(test_io)

add_subdirectory(test_bind)

add_subdirectory(test_invoke)

add_subdirectory(test_container)

add_subdirectory(test_gc)

add_subdirectory(test_operators)

add_subdirectory(test_concurrent)
