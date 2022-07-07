
include(GNUInstallDirs)

install(
    PROGRAMS "${CMAKE_BINARY_DIR}/${APP_FILE_NAME}"
    DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
install(
    FILES "${CMAKE_SOURCE_DIR}/data/${APP_FILE_NAME}.desktop"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
)

include(build_icons)
build_icons(
    "${CMAKE_BINARY_DIR}/icons"
    RASTER_SIZES all
    INCLUDE_SCALABLE
)
install(
    DIRECTORY "${CMAKE_BINARY_DIR}/icons"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/${APP_FILE_NAME}"
)

configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/install_unix_hicolor_icons.cmake.in"
    "${CMAKE_BINARY_DIR}/install_unix_hicolor_icons.cmake"
    @ONLY
)
install(
    SCRIPT
    "${CMAKE_BINARY_DIR}/install_unix_hicolor_icons.cmake"
)

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

# Construct doc dir manually since default DOCDIR uses PROJECT_NAME,
# which is in title case.
set(
    CMAKE_INSTALL_DOCDIR
    "${CMAKE_INSTALL_DATADIR}/doc/${APP_FILE_NAME}"
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
