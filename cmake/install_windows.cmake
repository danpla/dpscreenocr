
if (MINGW)
    include(install_windows_msys2)
endif()

install(
    PROGRAMS "${CMAKE_BINARY_DIR}/${APP_FILE_NAME}.exe"
    DESTINATION .
    COMPONENT Required
)

file(GLOB DLLS "${CMAKE_BINARY_DIR}/*.dll")
install(FILES ${DLLS} DESTINATION .)

install(DIRECTORY "${CMAKE_BINARY_DIR}/platforms" DESTINATION .)

install(
    DIRECTORY "${CMAKE_BINARY_DIR}/tessdata"
    DESTINATION .
    COMPONENT Required
)

if (DPSO_ENABLE_NLS)
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

if (DPSO_GEN_HTML_MANUAL)
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
