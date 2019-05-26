
# Instructions specific to MSYS2/MinGW.


# Copy tessdata directory from share/ to dst_dir.
function(copy_tessdata dst_dir)
    set(SRC_DIR "$ENV{MINGW_PREFIX}/share/tessdata")

    add_custom_command(
        OUTPUT "${dst_dir}/tessdata"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${SRC_DIR}" "${dst_dir}/tessdata"
        DEPENDS "${SRC_DIR}"
        VERBATIM
    )

    add_custom_target("tessdata_dir" ALL DEPENDS "${dst_dir}/tessdata")
endfunction()


copy_tessdata("${CMAKE_BINARY_DIR}")


if (DPSO_UI STREQUAL "qt")
    include(qt_utils)
    include(get_linguas)

    if (DPSO_COMPILE_PO)
        get_linguas(LANGS)
        copy_qt_translations(
            "$ENV{MINGW_PREFIX}/share/qt5/translations"
            "${CMAKE_BINARY_DIR}/translations"
            LANGUAGES ${LANGS}
            COMPONENTS qt qtbase
        )
        install(DIRECTORY "translations" DESTINATION .)
    endif()
endif()
