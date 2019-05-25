if (DPSO_COMPILE_PO)
    include(compile_po)
    compile_po(mo_files "${CMAKE_BINARY_DIR}/locale")

    install(
        # Note the trailing slash.
        DIRECTORY "${CMAKE_BINARY_DIR}/locale/"
        DESTINATION "${CMAKE_INSTALL_LOCALEDIR}"
    )
endif()
