
find_program(MSGFMT_EXE msgfmt)
if (NOT MSGFMT_EXE)
    message(SEND_ERROR "msgfmt not found")
endif()

# The function compiles PO files for languages listed in po/LINGUAS,
# writing MOs to build_dir.
#
# The full path of a MO file will be:
#
#   build_dir/{LANGUAGE}/LC_MESSAGES/${APP_FILE_NAME}.mo
function(compile_po target_name build_dir)
    set(MO_FILES)
    file(STRINGS "${CMAKE_SOURCE_DIR}/po/LINGUAS" LANGS REGEX "^[^#].*")
    foreach(LANG ${LANGS})
        set(PO_FILE "${CMAKE_SOURCE_DIR}/po/${LANG}.po")
        set(MO_DIR "${build_dir}/${LANG}/LC_MESSAGES")
        set(MO_FILE "${MO_DIR}/${APP_FILE_NAME}.mo")

        # msgfmt doesn't create intermediate directories.
        add_custom_command(
            OUTPUT "${MO_DIR}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${MO_DIR}"
            VERBATIM
        )

        add_custom_command(
            OUTPUT "${MO_FILE}"
            COMMAND "${MSGFMT_EXE}" -o "${MO_FILE}" "${PO_FILE}"
            DEPENDS "${MO_DIR}" "${PO_FILE}"
            VERBATIM
        )

        list(APPEND MO_FILES "${MO_FILE}")
    endforeach()

    add_custom_target("${target_name}" ALL DEPENDS ${MO_FILES})
endfunction()
