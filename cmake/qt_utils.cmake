# Install standard Qt translations from SRC_DIR to DST_DIR.
#
# install_qt_translations(
#   SRC_DIR
#   DST_DIR
#   LANGUAGES languages...
#   COMPONENTS components...)
function(install_qt_translations SRC_DIR DST_DIR)
    cmake_parse_arguments(ARG "" "" "LANGUAGES;COMPONENTS" ${ARGN})

    set(QMS)
    foreach(LANG ${ARG_LANGUAGES})
        foreach(COMPONENT ${ARG_COMPONENTS})
            set(QM "${SRC_DIR}/${COMPONENT}_${LANG}.qm")
            if(EXISTS "${QM}")
                list(APPEND QMS "${QM}")
            endif()
        endforeach()
    endforeach()

    if(QMS)
        install(FILES ${QMS} DESTINATION "${DST_DIR}")
    endif()
endfunction()

function(install_qt_windows_plugins SRC_DIR DST_DIR)
    set(PLUGINS
        "platforms/qwindows.dll")

    if(EXISTS "${SRC_DIR}/styles/qwindowsvistastyle.dll")
        list(APPEND PLUGINS "styles/qwindowsvistastyle.dll")
    else()
        # Qt 6.7 and newer.
        list(APPEND PLUGINS "styles/qmodernwindowsstyle.dll")
    endif()

    foreach(PLUGIN ${PLUGINS})
        set(SRC_FILE "${SRC_DIR}/${PLUGIN}")
        if(NOT EXISTS "${SRC_FILE}")
            message(FATAL_ERROR "\"${SRC_FILE}\" does not exist")
        endif()

        cmake_path(GET PLUGIN PARENT_PATH PLUGIN_DIR)
        set(DST_FILE_DIR "${DST_DIR}/${PLUGIN_DIR}")

        install(FILES "${SRC_FILE}" DESTINATION "${DST_FILE_DIR}")
    endforeach()
endfunction()
