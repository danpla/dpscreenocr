
include(GNUInstallDirs)


install(
    PROGRAMS "${CMAKE_BINARY_DIR}/${APP_FILE_NAME}"
    DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
install(
    FILES "${CMAKE_SOURCE_DIR}/data/${APP_FILE_NAME}.desktop"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
)
install(
    DIRECTORY "data/icons/hicolor"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/icons"
)

if(DPSO_QT_LOCAL_DATA AND DPSO_UI STREQUAL "qt")
    include(build_icons)
    build_icons(
        "${CMAKE_BINARY_DIR}/icons"
        RASTER_SIZES all
        INCLUDE_SCALABLE
    )
endif()

if(DPSO_ENABLE_NLS)
    include(compile_po)
    compile_po(
        mo_files "${CMAKE_BINARY_DIR}/locale" "${APP_FILE_NAME}"
    )

    install(
        # Note the trailing slash.
        DIRECTORY "${CMAKE_BINARY_DIR}/locale/"
        DESTINATION "${CMAKE_INSTALL_LOCALEDIR}"
    )
endif()

# DOCDIR uses PROJECT_NAME, which is in title case.
string(
    REPLACE
    "${PROJECT_NAME}" "${APP_FILE_NAME}"
    CMAKE_INSTALL_DOCDIR ${CMAKE_INSTALL_DOCDIR}
)
if(DPSO_GEN_HTML_MANUAL)
    include(gen_manual)
    gen_html_manual(html_manual "${CMAKE_BINARY_DIR}/manual")

    install(
        # Note the trailing slash.
        DIRECTORY "${CMAKE_BINARY_DIR}/manual/"
        DESTINATION "${CMAKE_INSTALL_DOCDIR}"
    )
else()
    install(
        FILES "${CMAKE_SOURCE_DIR}/doc/manual.md"
        DESTINATION "${CMAKE_INSTALL_DOCDIR}"
        RENAME "manual.txt"
    )
endif()

install(
    FILES "${CMAKE_SOURCE_DIR}/doc/changelog.txt"
    DESTINATION "${CMAKE_INSTALL_DOCDIR}"
)

install(
    FILES "${CMAKE_SOURCE_DIR}/LICENSE.txt"
    DESTINATION "${CMAKE_INSTALL_DOCDIR}"
)
