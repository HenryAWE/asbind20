macro(asbind20_setup_extension ext_target)
    # Add an ALIAS target for using the library by add_subdirectory()
    add_library(asbind20::${ext_target} ALIAS ${ext_target})

    install(
        TARGETS ${ext_target}
        EXPORT asbind20Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.hpp" PATTERN "*.inl"
    )
endmacro()

add_subdirectory(container)
add_subdirectory(utility)
add_subdirectory(testing)
