include(GoogleTest)
include(asbindTestFramework)

add_executable(test_load test_load.cpp)
target_link_libraries(test_load PRIVATE asbind20_ext_utility)
target_link_libraries(test_load PRIVATE shared_test_lib)
append_script_to_executable(test_load "script/func.as")
gtest_discover_tests(test_load)
