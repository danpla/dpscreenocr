
include(CMakeParseArguments)

# Copy standard Qt translations from src_dir to dst_dir.
#
# copy_qt_translations(
#   src_dir dst_dir
#   LANGUAGES languages...
#   COMPONENTS components...)
function(copy_qt_translations src_dir dst_dir)
    cmake_parse_arguments(ARG "" "" "LANGUAGES;COMPONENTS" ${ARGN})

    set(SRC_QMS)
    foreach(LANG ${ARG_LANGUAGES})
        foreach(COMPONENT ${ARG_COMPONENTS})
            set(QM "${src_dir}/${COMPONENT}_${LANG}.qm")
            if(EXISTS "${QM}")
                list(APPEND SRC_QMS "${QM}")
            endif()
        endforeach()
    endforeach()

    set(DST_QMS)
    foreach(SRC_QM ${SRC_QMS})
        get_filename_component(QM_NAME "${SRC_QM}" NAME)
        set(DST_QM "${dst_dir}/${QM_NAME}")
        list(APPEND DST_QMS "${DST_QM}")

        add_custom_command(
            OUTPUT "${DST_QM}"
            COMMAND ${CMAKE_COMMAND} -E copy "${SRC_QM}" "${DST_QM}"
            DEPENDS "${SRC_QM}"
            VERBATIM
        )
    endforeach()

    add_custom_target("qt_translations" ALL DEPENDS ${DST_QMS})
endfunction()

function(copy_qt_windows_plugins src_dir dst_dir)
    set(
        PLUGINS
        "platforms/qwindows.dll"
        "styles/qwindowsvistastyle.dll"
    )

    set(TARGET_DEPENDENCIES)

    foreach(PLUGIN ${PLUGINS})
        set(SRC_FILE "${src_dir}/${PLUGIN}")
        set(DST_FILE "${dst_dir}/${PLUGIN}")

        add_custom_command(
            OUTPUT "${DST_FILE}"
            COMMAND ${CMAKE_COMMAND} -E copy "${SRC_FILE}" "${DST_FILE}"
            DEPENDS "${SRC_FILE}"
            VERBATIM
        )
        list(APPEND TARGET_DEPENDENCIES "${DST_FILE}")
    endforeach()

    add_custom_target(
        "qt_windows_plugins" ALL DEPENDS ${TARGET_DEPENDENCIES}
    )
endfunction()
