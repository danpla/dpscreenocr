# Copy standard Qt translations from SRC_DIR to DST_DIR.
#
# copy_qt_translations(
#   SRC_DIR
#   DST_DIR
#   LANGUAGES languages...
#   COMPONENTS components...)
function(copy_qt_translations SRC_DIR DST_DIR)
    cmake_parse_arguments(ARG "" "" "LANGUAGES;COMPONENTS" ${ARGN})

    set(SRC_QMS)
    foreach(LANG ${ARG_LANGUAGES})
        foreach(COMPONENT ${ARG_COMPONENTS})
            set(QM "${SRC_DIR}/${COMPONENT}_${LANG}.qm")
            if(EXISTS "${QM}")
                list(APPEND SRC_QMS "${QM}")
            endif()
        endforeach()
    endforeach()

    set(DST_QMS)
    foreach(SRC_QM ${SRC_QMS})
        get_filename_component(QM_NAME "${SRC_QM}" NAME)
        set(DST_QM "${DST_DIR}/${QM_NAME}")
        list(APPEND DST_QMS "${DST_QM}")

        add_custom_command(
            OUTPUT "${DST_QM}"
            COMMAND "${CMAKE_COMMAND}" -E copy "${SRC_QM}" "${DST_QM}"
            DEPENDS "${SRC_QM}"
            VERBATIM)
    endforeach()

    add_custom_target(qt_translations ALL DEPENDS ${DST_QMS})
endfunction()

function(copy_qt_windows_plugins SRC_DIR DST_DIR)
    set(PLUGINS
        "platforms/qwindows.dll"
        "styles/qwindowsvistastyle.dll")

    set(DST_FILES)

    foreach(PLUGIN ${PLUGINS})
        set(SRC_FILE "${SRC_DIR}/${PLUGIN}")
        set(DST_FILE "${DST_DIR}/${PLUGIN}")

        add_custom_command(
            OUTPUT "${DST_FILE}"
            COMMAND
                "${CMAKE_COMMAND}" -E copy "${SRC_FILE}" "${DST_FILE}"
            DEPENDS "${SRC_FILE}"
            VERBATIM)
        list(APPEND DST_FILES "${DST_FILE}")
    endforeach()

    add_custom_target(
        qt_windows_plugins ALL DEPENDS ${DST_FILES})
endfunction()
