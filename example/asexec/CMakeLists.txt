add_executable(asexec "asexec.cpp")
if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

target_link_libraries(asexec PRIVATE asbind20)
target_link_libraries(asexec PRIVATE asbind20_ext_utility)
target_link_libraries(asexec PRIVATE asbind20_ext_container)
