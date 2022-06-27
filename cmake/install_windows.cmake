
if(MINGW)
    include(install_windows_msys2)
endif()

install(
    PROGRAMS "${CMAKE_BINARY_DIR}/${APP_FILE_NAME}.exe"
    DESTINATION .
    COMPONENT Required
)

configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/install_windows_dlls.cmake.in"
    "${CMAKE_BINARY_DIR}/install_windows_dlls.cmake"
    @ONLY
)
install(
    SCRIPT "${CMAKE_BINARY_DIR}/install_windows_dlls.cmake"
    COMPONENT Required
)

if(DPSO_UI STREQUAL "qt")
    include(build_icons)
    build_icons(
        "${CMAKE_BINARY_DIR}/icons"
        RASTER_SIZES all
    )

    install(
        DIRECTORY
            "${CMAKE_BINARY_DIR}/icons"
            "${CMAKE_BINARY_DIR}/platforms"
            "${CMAKE_BINARY_DIR}/styles"
        DESTINATION .
        COMPONENT Required
    )

    if(DPSO_ENABLE_NLS)
        install(
            DIRECTORY "${CMAKE_BINARY_DIR}/translations"
            DESTINATION .
            COMPONENT localization
        )
    endif()
endif()

include(tesseract_utils)
get_tesseract_data_dir_name(TESSERACT_DATA_DIR_NAME)
if(TESSERACT_DATA_DIR_NAME)
    install(
        DIRECTORY
        DESTINATION "${TESSERACT_DATA_DIR_NAME}"
        COMPONENT Required
    )
    install(
        FILES "${CMAKE_BINARY_DIR}/${TESSERACT_DATA_DIR_NAME}/eng.traineddata"
        DESTINATION "${TESSERACT_DATA_DIR_NAME}"
        COMPONENT Required
        OPTIONAL
    )
else()
    message("Tesseract data dir was not detected; please set up Tesseract data manually")
endif()

if(DPSO_ENABLE_NLS)
    include(compile_po)
    compile_po(
        mo_files "${CMAKE_BINARY_DIR}/locale" "${APP_FILE_NAME}"
    )

    install(
        DIRECTORY "${CMAKE_BINARY_DIR}/locale"
        DESTINATION .
        COMPONENT localization
    )
endif()

include(line_endings_conversion)

if(DPSO_GEN_HTML_MANUAL)
    include(gen_manual)
    gen_html_manual(html_manual "${CMAKE_BINARY_DIR}/doc")
else()
    unix2dos(
        "unix2dos_manual"
        "${CMAKE_SOURCE_DIR}/doc/manual.md"
        "${CMAKE_BINARY_DIR}/doc/manual.txt"
    )
endif()

unix2dos(
    "unix2dos_changelog"
    "${CMAKE_SOURCE_DIR}/doc/changelog.txt"
    "${CMAKE_BINARY_DIR}/doc/changelog.txt"
)

unix2dos(
    "unix2dos_license"
    "${CMAKE_SOURCE_DIR}/LICENSE.txt"
    "${CMAKE_BINARY_DIR}/doc/license.txt"
)

install(
    DIRECTORY "${CMAKE_BINARY_DIR}/doc"
    DESTINATION .
    COMPONENT Required
)
