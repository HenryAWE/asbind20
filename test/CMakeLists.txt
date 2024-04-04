include(GoogleTest)

add_executable(test_invoke test_invoke.cpp)
target_link_libraries(test_invoke PRIVATE asbind20)
target_link_libraries(test_invoke PRIVATE GTest::gtest GTest::gtest_main)
gtest_discover_tests(test_invoke)
