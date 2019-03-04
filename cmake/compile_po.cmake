
find_package(Gettext REQUIRED)

# The function compiles PO files for languages listed in po/LINGUAS,
# writing MOs to build_dir, and optionally installing them to
# install_dir. If you don't need to install MOs, leave install_dir
# empty.
#
# The full path of a MO file will be build_dir/install_dir +
# "${LANG}/LC_MESSAGES/${APP_FILE_NAME}.mo".
function(COMPILE_PO target_name build_dir install_dir)
    set(MO_FILES)
    file(STRINGS "po/LINGUAS" LANGS REGEX "^[^#].*")
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
            COMMAND
                "${GETTEXT_MSGFMT_EXECUTABLE}" -o "${MO_FILE}" "${PO_FILE}"
            DEPENDS "${MO_DIR}" "${PO_FILE}"
            VERBATIM
        )

        if (NOT install_dir STREQUAL "")
            install(
                FILES "${MO_FILE}"
                DESTINATION "${install_dir}/${LANG}/LC_MESSAGES"
            )
        endif()

        list(APPEND MO_FILES "${MO_FILE}")
    endforeach()

    add_custom_target("${target_name}" ALL DEPENDS ${MO_FILES})
endfunction()
