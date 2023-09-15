
# Generates a desktop entry "${APP_FILE_NAME}.desktop" in DST_DIR. If
# TRANSLATE is TRUE, the entry will be translated using msgfmt based
# on files from the po/ directory.
function(gen_desktop_entry DST_DIR TRANSLATE)
    set(ENTRY_NAME "${APP_FILE_NAME}.desktop")

    set(DST_DEP_VARS_FILE_PATH "${DST_DIR}/${ENTRY_NAME}.dep_vars")
    configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/gen_desktop_entry.dep_vars.in"
        "${DST_DEP_VARS_FILE_PATH}"
        @ONLY)

    set(SRC_ENTRY_PATH "${CMAKE_SOURCE_DIR}/data/${ENTRY_NAME}")
    set(DST_ENTRY_PATH "${DST_DIR}/${ENTRY_NAME}")

    if(NOT TRANSLATE)
        configure_file(
            "${SRC_ENTRY_PATH}" "${DST_ENTRY_PATH}" COPYONLY)
        return()
    endif()

    find_program(MSGFMT_EXE msgfmt)
    if(NOT MSGFMT_EXE)
        message(SEND_ERROR "msgfmt not found")
    endif()

    add_custom_command(
        OUTPUT "${DST_ENTRY_PATH}"
        # msgfmt doesn't create intermediate directories.
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DST_DIR}"
        COMMAND
            "${MSGFMT_EXE}"
            --desktop
            # Disable default keywords since we only want "Comment" to
            # be translated. As of version 0.21, msgfmt has two bugs:
            # -k is not a recognized option, and --keyword without
            # argument doesn't work (so we use an explicit empty
            # string as a workaround).
            --keyword=
            --keyword=Comment
            "--template=${SRC_ENTRY_PATH}"
            -d "${CMAKE_SOURCE_DIR}/po"
            -o "${DST_ENTRY_PATH}"
        DEPENDS "${SRC_ENTRY_PATH}" "${DST_DEP_VARS_FILE_PATH}"
        VERBATIM)

    add_custom_target(desktop_entry ALL DEPENDS "${DST_ENTRY_PATH}")
endfunction()
