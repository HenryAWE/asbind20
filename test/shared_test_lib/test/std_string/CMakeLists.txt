include(asbindTestFramework)

aux_source_directory(. test_script_string_src)

add_executable(test_script_string ${test_script_string_src})
target_link_libraries(test_script_string PRIVATE shared_test_lib)
append_script_to_executable(test_script_string "script/test_string.as")
gtest_discover_tests(test_script_string DISCOVERY_TIMEOUT 1000)
