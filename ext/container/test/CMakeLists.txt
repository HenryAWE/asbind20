include(GoogleTest)

add_subdirectory("test_ext_array")

add_executable(test_ext_stdstring test_ext_stdstring.cpp)
target_link_libraries(test_ext_stdstring PRIVATE shared_test_lib)
gtest_discover_tests(test_ext_stdstring)
if(EMSCRIPTEN)
    set_target_properties(test_ext_stdstring PROPERTIES LINK_FLAGS "--preload-file ${CMAKE_CURRENT_SOURCE_DIR}/script/test_string.as@script/test_string.as")
    set_target_properties(test_ext_stdstring PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/script/test_string.as)
else()
    configure_file(script/test_string.as ${CMAKE_CURRENT_BINARY_DIR}/script/test_string.as COPYONLY)
endif()
