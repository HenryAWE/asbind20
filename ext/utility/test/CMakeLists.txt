include(GoogleTest)

add_executable(test_ext_math test_ext_math.cpp)
target_link_libraries(test_ext_math PRIVATE shared_test_lib)
gtest_discover_tests(test_ext_math)
configure_file(script/test_math.as ${CMAKE_CURRENT_BINARY_DIR}/script/test_math.as COPYONLY)
