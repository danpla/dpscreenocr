include(get_linguas)

find_program(MSGFMT_EXE msgfmt REQUIRED)

# The function compiles PO files for languages listed in po/LINGUAS,
# writing MOs to DST_DIR.
#
# The full path of a MO file will be:
#
#   DST_DIR/{LANGUAGE}/LC_MESSAGES/${MO_NAME}.mo
function(compile_po DST_DIR MO_NAME)
    set(MO_FILES)
    get_linguas(LANGS)
    foreach(LANG ${LANGS})
        set(PO_FILE "${CMAKE_SOURCE_DIR}/po/${LANG}.po")
        set(MO_DIR "${DST_DIR}/${LANG}/LC_MESSAGES")
        set(MO_FILE "${MO_DIR}/${MO_NAME}.mo")

        add_custom_command(
            OUTPUT "${MO_FILE}"
            # msgfmt doesn't create intermediate directories.
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${MO_DIR}"
            COMMAND "${MSGFMT_EXE}" -o "${MO_FILE}" "${PO_FILE}"
            DEPENDS "${PO_FILE}"
            VERBATIM)

        list(APPEND MO_FILES "${MO_FILE}")
    endforeach()

    add_custom_target(mo_files ALL DEPENDS ${MO_FILES})
endfunction()
