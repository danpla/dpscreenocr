
find_program(UNIX2DOS_EXE unix2dos)
if (NOT UNIX2DOS_EXE)
    message(SEND_ERROR "unix2dos not found")
endif()


# Convert line endings from Unix (LF) to Windows/DOS (CRLF).
function(unix2dos target_name in_file out_file)
    get_filename_component(OUT_DIR "${out_file}" PATH)

    add_custom_command(
        OUTPUT "${out_file}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${OUT_DIR}"
        COMMAND "${UNIX2DOS_EXE}" -q -n "${in_file}" "${out_file}"
        DEPENDS "${in_file}"
        VERBATIM
    )

    add_custom_target("${target_name}" ALL DEPENDS "${out_file}")
endfunction()
