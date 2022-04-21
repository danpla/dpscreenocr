
# There is a bug in NSIS that doesn't handle absolute paths that
# contain a forward slash before the final component for some
# variables (like MUI_WELCOMEFINISHPAGE_BITMAP). We thus should use a
# backslash instead.


if(CMAKE_VERSION VERSION_LESS 3.17)
    # CPACK_NSIS_MUI_HEADERIMAGE requires 3.17
    message(WARNING "Use CMake 3.17 or newer to enable all features of NSIS installer")
endif()

include(dist_installer_common)

set(CPACK_NSIS_PACKAGE_NAME "${APP_NAME} ${APP_VERSION}")
set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")

# Start menu page defaults to CPACK_NSIS_PACKAGE_NAME, which contains
# the program version.
set(
    CPACK_NSIS_DEFINES
    "  !define MUI_STARTMENUPAGE_DEFAULTFOLDER \\\"${APP_NAME}\\\""
)

set(CPACK_NSIS_INSTALLED_ICON_NAME "${APP_FILE_NAME}.exe")
set(
    CPACK_NSIS_MUI_ICON
    "${CMAKE_SOURCE_DIR}/data/icons/${APP_FILE_NAME}.ico"
)

set(
    CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP
    "${CMAKE_SOURCE_DIR}/data/nsis\\\\win.bmp"
)

set(
    CPACK_NSIS_MUI_UNWELCOMEFINISHPAGE_BITMAP
    "${CMAKE_SOURCE_DIR}/data/nsis\\\\win.bmp"
)

set(
    CPACK_NSIS_MUI_HEADERIMAGE
    "${CMAKE_SOURCE_DIR}/data/nsis\\\\header.bmp"
)

set(CPACK_NSIS_HELP_LINK "${APP_URL}")
set(CPACK_NSIS_URL_INFO_ABOUT "${APP_URL}")

set(CPACK_NSIS_MENU_LINKS "doc" "Documents")

set(CPACK_NSIS_MUI_FINISHPAGE_RUN "${APP_FILE_NAME}.exe")

set(CPACK_NSIS_COMPRESSOR "/SOLID lzma")
