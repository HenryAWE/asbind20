aux_source_directory(src asbind20_ext_container_src)
add_library(asbind20_ext_container ${asbind20_ext_container_src})
target_link_libraries(asbind20_ext_container PUBLIC asbind20)
target_link_libraries(asbind20_ext_container PUBLIC asbind20_ext_utility)
target_include_directories(asbind20_ext_container PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(${asbind_build_test})
    add_subdirectory(test)
endif()

asbind20_setup_extension(asbind20_ext_container)
