
if (DPSO_UI STREQUAL "qt")
    include(qt_utils)

    if (DPSO_COMPILE_PO)
        copy_qt_translations(
            "$ENV{MINGW_PREFIX}/share/qt5/translations"
            "${CMAKE_BINARY_DIR}/translations"
            COMPONENTS qt qtbase
        )
        install(DIRECTORY "translations" DESTINATION .)
    endif()
endif()
