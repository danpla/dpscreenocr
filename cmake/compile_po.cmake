
include(get_linguas)

find_program(MSGFMT_EXE msgfmt)
if (NOT MSGFMT_EXE)
    message(SEND_ERROR "msgfmt not found")
endif()

# The function compiles PO files for languages listed in po/LINGUAS,
# writing MOs to dst_dir.
#
# The full path of a MO file will be:
#
#   dst_dir/{LANGUAGE}/LC_MESSAGES/${APP_FILE_NAME}.mo
function(compile_po target_name dst_dir)
    set(MO_FILES)
    get_linguas(LANGS)
    foreach(LANG ${LANGS})
        set(PO_FILE "${CMAKE_SOURCE_DIR}/po/${LANG}.po")
        set(MO_DIR "${dst_dir}/${LANG}/LC_MESSAGES")
        set(MO_FILE "${MO_DIR}/${APP_FILE_NAME}.mo")

        add_custom_command(
            OUTPUT "${MO_FILE}"
            # msgfmt doesn't create intermediate directories.
            COMMAND ${CMAKE_COMMAND} -E make_directory "${MO_DIR}"
            COMMAND "${MSGFMT_EXE}" -o "${MO_FILE}" "${PO_FILE}"
            DEPENDS "${PO_FILE}"
            VERBATIM
        )

        list(APPEND MO_FILES "${MO_FILE}")
    endforeach()

    add_custom_target("${target_name}" ALL DEPENDS ${MO_FILES})
endfunction()
