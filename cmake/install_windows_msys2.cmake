
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

function(copy_qt5_plugins dst_dir)
    set(SRC_DIR "$ENV{MINGW_PREFIX}/share/qt5/plugins")

    add_custom_command(
        OUTPUT "${dst_dir}/platforms" "${dst_dir}/styles"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${dst_dir}/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy "${SRC_DIR}/platforms/qwindows.dll" "${dst_dir}/platforms"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${dst_dir}/styles"
        COMMAND ${CMAKE_COMMAND} -E copy "${SRC_DIR}/styles/qwindowsvistastyle.dll" "${dst_dir}/styles"
        DEPENDS "${SRC_DIR}/platforms/qwindows.dll" "${SRC_DIR}/styles/qwindowsvistastyle.dll"
        VERBATIM
    )

    add_custom_target("qt5_plugins" ALL DEPENDS "${dst_dir}/platforms" "${dst_dir}/styles")
endfunction()

if(DPSO_UI STREQUAL "qt")
    copy_qt5_plugins("${CMAKE_BINARY_DIR}")

    include(qt_utils)
    include(get_linguas)

    if(DPSO_ENABLE_NLS)
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
