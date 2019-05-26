
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
