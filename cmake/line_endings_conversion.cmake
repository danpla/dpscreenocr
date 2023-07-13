
# We don't use CMake's configure_file() since NEWLINE_STYLE option
# cannot be used together with COPYONLY that disables variable
# substitution.

find_program(UNIX2DOS_EXE unix2dos)
if(NOT UNIX2DOS_EXE)
    message(SEND_ERROR "unix2dos not found")
endif()

# Convert line endings from Unix (LF) to Windows/DOS (CRLF).
function(unix2dos TARGET_NAME IN_FILE OUT_FILE)
    get_filename_component(OUT_DIR "${OUT_FILE}" PATH)

    add_custom_command(
        OUTPUT "${OUT_FILE}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${OUT_DIR}"
        COMMAND "${UNIX2DOS_EXE}" -q -n "${IN_FILE}" "${OUT_FILE}"
        DEPENDS "${IN_FILE}"
        VERBATIM)

    add_custom_target("${TARGET_NAME}" ALL DEPENDS "${OUT_FILE}")
endfunction()
