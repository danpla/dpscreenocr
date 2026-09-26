# Instructions specific to MSYS2/MinGW.

install(
    RUNTIME_DEPENDENCY_SET "${APP_FILE_NAME}"
    DESTINATION .
    DIRECTORIES "$ENV{MINGW_PREFIX}/bin"
    # As of version 3.23, CMake doesn't seem to handle Windows API
    # sets, so we filter them manually via PRE_EXCLUDE_REGEXES.
    # https://docs.microsoft.com/en-us/windows/win32/apiindex/windows-apisets
    # https://gitlab.kitware.com/cmake/cmake/-/issues/22006
    PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*"
    POST_INCLUDE_REGEXES "^$ENV{MINGW_PREFIX}/bin/.*"
    POST_EXCLUDE_REGEXES ".*")

include(tesseract_utils)
get_tesseract_data_dir_name(TESSERACT_DATA_DIR_NAME)
if(TESSERACT_DATA_DIR_NAME)
    copy_tessdata(
        "$ENV{MINGW_PREFIX}/share/tessdata"
        "${CMAKE_BINARY_DIR}/${TESSERACT_DATA_DIR_NAME}"
        LANGUAGES eng
        OPTIONAL)
endif()

if(DPSO_UI STREQUAL "qt")
    include(qt_utils)

    install_qt_windows_plugins(
        "$ENV{MINGW_PREFIX}/share/qt${DPSO_QT_VERSION}/plugins"
        "qt${DPSO_QT_VERSION}/plugins")

    include(get_linguas)
    get_linguas(LANGS)

    install_qt_translations(
        "$ENV{MINGW_PREFIX}/share/qt${DPSO_QT_VERSION}/translations"
        "qt${DPSO_QT_VERSION}/translations"
        LANGUAGES ${LANGS}
        COMPONENTS qt qtbase)
endif()
