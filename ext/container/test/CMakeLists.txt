include(GoogleTest)

add_executable(test_ext_array test_ext_array.cpp)
target_link_libraries(test_ext_array PRIVATE shared_test_lib)
gtest_discover_tests(test_ext_array)

add_executable(test_ext_stdstring test_ext_stdstring.cpp)
target_link_libraries(test_ext_stdstring PRIVATE shared_test_lib)
gtest_discover_tests(test_ext_stdstring)
configure_file(script/test_string.as ${CMAKE_CURRENT_BINARY_DIR}/script/test_string.as COPYONLY)
