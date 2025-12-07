include(GoogleTest)
include(asbindTestFramework)

add_executable(test_section test_section.cpp)
target_link_libraries(test_section PRIVATE shared_test_lib)
append_script_to_executable(test_section "script/func.as")
gtest_discover_tests(test_section DISCOVERY_TIMEOUT 1000)
