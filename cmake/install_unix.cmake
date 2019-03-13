
include(GNUInstallDirs)


install(
    PROGRAMS "${CMAKE_BINARY_DIR}/${APP_FILE_NAME}"
    DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/data/${APP_FILE_NAME}.desktop"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
)
install(
    DIRECTORY "data/icons/hicolor"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/icons"
)

if (DPSO_COMPILE_PO)
    include(compile_po)

    COMPILE_PO(
        mo_files
        "${CMAKE_CURRENT_BINARY_DIR}/locale"
    )
    install(
        # Note the trailing slash.
        DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/locale/"
        DESTINATION "${CMAKE_INSTALL_LOCALEDIR}"
    )
endif()
