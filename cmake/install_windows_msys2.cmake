
# Instructions specific to MSYS2/MinGW.

if(NOT CMAKE_VERSION VERSION_LESS 3.16)
    configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/install_windows_msys2_copy_dlls.cmake.in"
        "${CMAKE_BINARY_DIR}/install_windows_msys2_copy_dlls.cmake"
        @ONLY
    )

    add_custom_target(
        "copy_dlls"
        ALL
        COMMENT "Copying DLLs"
        COMMAND ${CMAKE_COMMAND} -P "${CMAKE_BINARY_DIR}/install_windows_msys2_copy_dlls.cmake"
    )

    add_dependencies("copy_dlls" "${APP_FILE_NAME}_${DPSO_UI}")
else()
    message(WARNING "DLLs are not copied automatically because CMake version is < 3.16")
endif()

include(tessdata_utils)
copy_tessdata(
    "$ENV{MINGW_PREFIX}/share/tessdata"
    "${CMAKE_BINARY_DIR}/tessdata"
    LANGUAGES eng
    OPTIONAL
)

if(DPSO_UI STREQUAL "qt")
    include(qt_utils)

    copy_qt_windows_plugins(
        "$ENV{MINGW_PREFIX}/share/qt5/plugins" "${CMAKE_BINARY_DIR}"
    )

    if(DPSO_ENABLE_NLS)
        include(get_linguas)
        get_linguas(LANGS)

        copy_qt_translations(
            "$ENV{MINGW_PREFIX}/share/qt5/translations"
            "${CMAKE_BINARY_DIR}/translations"
            LANGUAGES ${LANGS}
            COMPONENTS qt qtbase
        )
        install(
            DIRECTORY "${CMAKE_BINARY_DIR}/translations"
            DESTINATION .
            COMPONENT localization
        )
    endif()
endif()
