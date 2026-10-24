install(
    TARGETS "${APP_FILE_NAME}"
    RUNTIME_DEPENDENCY_SET "${APP_FILE_NAME}"
    DESTINATION .)

if(MINGW)
    include(install_windows_msys2)
endif()

if(DPSO_UI STREQUAL "qt")
    include(icon_utils)
    install_icons("icons" RASTER_SIZES all)

    string(
        JOIN "\n" QT_CONF
        "[Paths]"
        "Prefix = qt${DPSO_QT_VERSION}")
    file(
        GENERATE
        OUTPUT "${CMAKE_BINARY_DIR}/qt.conf"
        CONTENT "${QT_CONF}")
    install(
        FILES "${CMAKE_BINARY_DIR}/qt.conf"
        DESTINATION .)
endif()

install(
    DIRECTORY "${CMAKE_SOURCE_DIR}/data/sounds"
    DESTINATION .)

include(compile_po)
compile_po("${CMAKE_BINARY_DIR}/translations")
install(
    DIRECTORY "${CMAKE_BINARY_DIR}/translations"
    DESTINATION .)

include(line_endings_conversion)

if(DPSO_GEN_HTML_MANUAL)
    include(gen_manual)
    gen_html_manual("${CMAKE_BINARY_DIR}/doc")
else()
    convert_line_endings(
        "${CMAKE_SOURCE_DIR}/doc/manual.md"
        "${CMAKE_BINARY_DIR}/doc/manual.txt"
        CRLF)
endif()

convert_line_endings(
    "${CMAKE_SOURCE_DIR}/doc/changelog.txt"
    "${CMAKE_BINARY_DIR}/doc/changelog.txt"
    CRLF)

convert_line_endings(
    "${CMAKE_SOURCE_DIR}/LICENSE.txt"
    "${CMAKE_BINARY_DIR}/doc/license.txt"
    CRLF)

set(TPL_DIR_NAME "third-party-licenses")
if(NOT EXISTS "${CMAKE_BINARY_DIR}/doc/${TPL_DIR_NAME}")
    message(
        WARNING
        "The optional \"${TPL_DIR_NAME}\" directory does not exists "
        "in \"${CMAKE_BINARY_DIR}/doc\"; it must be set up "
        "manually")
endif()

install(
    DIRECTORY "${CMAKE_BINARY_DIR}/doc"
    DESTINATION .)
