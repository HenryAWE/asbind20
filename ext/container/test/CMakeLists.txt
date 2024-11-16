include(GoogleTest)

add_executable(test_ext_array test_ext_array.cpp)
target_link_libraries(test_ext_array PRIVATE shared_test_lib)
gtest_discover_tests(test_ext_array)
configure_file(script/test_array.as ${CMAKE_CURRENT_BINARY_DIR}/script/test_array.as COPYONLY)

add_executable(test_ext_dictionary test_ext_dictionary.cpp)
target_link_libraries(test_ext_dictionary PRIVATE shared_test_lib)
gtest_discover_tests(test_ext_dictionary)

add_executable(test_ext_stdstring test_ext_stdstring.cpp)
target_link_libraries(test_ext_stdstring PRIVATE shared_test_lib)
gtest_discover_tests(test_ext_stdstring)
configure_file(script/test_string.as ${CMAKE_CURRENT_BINARY_DIR}/script/test_string.as COPYONLY)
