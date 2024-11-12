aux_source_directory(src asbind20_ext_utility_src)
add_library(asbind20_ext_utility ${asbind20_ext_utility_src})
target_link_libraries(asbind20_ext_utility PUBLIC asbind20)
target_include_directories(asbind20_ext_utility PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(${asbind_build_test})
    add_subdirectory(test)
endif()

asbind20_setup_extension(asbind20_ext_utility)
