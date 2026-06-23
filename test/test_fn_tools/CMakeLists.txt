file(GLOB_RECURSE test_fn_tools_src "*.cpp")

add_executable(test_fn_tools ${test_fn_tools_src})
target_link_libraries(test_fn_tools PRIVATE shared_test_lib)
gtest_discover_tests(test_fn_tools DISCOVERY_TIMEOUT 1000)
