
include(CMakeParseArguments)
include(get_linguas)


# Copy standard Qt translations from src_dir to dst_dir.
#
# copy_qt_translations(
#   src_dir dst_dir
#   COMPONENTS components...)
function(copy_qt_translations src_dir dst_dir)
    cmake_parse_arguments(ARG "" "" "COMPONENTS" ${ARGN})

    get_linguas(LANGS)

    set(SRC_QMS)
    foreach(LANG ${LANGS})
        foreach(COMPONENT ${ARG_COMPONENTS})
            set(QM "${src_dir}/${COMPONENT}_${LANG}.qm")
            if (NOT EXISTS "${QM}")
                continue()
            endif()

            list(APPEND SRC_QMS "${QM}")
        endforeach()
    endforeach()

    set(DST_QMS)
    foreach(SRC_QM ${SRC_QMS})
        get_filename_component(QM_NAME "${SRC_QM}" NAME)
        set(DST_QM "${dst_dir}/${QM_NAME}")
        list(APPEND DST_QMS "${DST_QM}")

        add_custom_command(
            OUTPUT "${DST_QM}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_QM}" "${DST_QM}"
            DEPENDS "${SRC_QM}"
            VERBATIM
        )
    endforeach()

    add_custom_target(
        "qt_translations"
        ALL
        DEPENDS ${DST_QMS}
    )
endfunction()
