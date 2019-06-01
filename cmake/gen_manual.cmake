
find_program(
    PANDOC_EXE pandoc
    # Allow to put pandoc.exe in the build directory. Makes life
    # simpler when building on Windows with MSYS2, as MSYS2 doesn't
    # inherit system PATH by default.
    PATHS "${CMAKE_BINARY_DIR}"
)
if (NOT PANDOC_EXE)
    message(SEND_ERROR "pandoc not found")
endif()


function(gen_html_manual target_name dst_dir)
    set(DOC_DIR "${CMAKE_SOURCE_DIR}/doc")

    set(DST_HTML "${dst_dir}/manual.html")
    separate_arguments(
        PANDOC_ARGS
        UNIX_COMMAND
        "--from=markdown --to=html5 --standalone --css=manual-data/manual.css --output=\"${DST_HTML}\" --template=\"${DOC_DIR}/manual-data/template.html\" --toc --number-sections -V \"title:dpScreenOCR Manual\" -V \"toctitle:Table of contents\" -V \"pagetitle:dpScreenOCR Manual\" \"${DOC_DIR}/manual.md\""
    )

    add_custom_command(
        OUTPUT "${DST_HTML}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${dst_dir}"
        COMMAND "${PANDOC_EXE}" ${PANDOC_ARGS}
        DEPENDS "${DOC_DIR}/manual.md" "${DOC_DIR}/manual-data/manual.css" "${DOC_DIR}/manual-data/template.html"
        VERBATIM
    )

    file(
        GLOB
        SRC_DATA_FILES
        "${DOC_DIR}/manual-data/*.png"
        "${DOC_DIR}/manual-data/*css"
    )

    set(DST_DATA_FILES)
    foreach(SRC_FILE ${SRC_DATA_FILES})
        get_filename_component(SRC_FILE_NAME "${SRC_FILE}" NAME)
        set(DST_FILE "${dst_dir}/manual-data/${SRC_FILE_NAME}")
        list(APPEND DST_DATA_FILES "${DST_FILE}")

        add_custom_command(
            OUTPUT "${DST_FILE}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SRC_FILE}" "${DST_FILE}"
            DEPENDS "${SRC_FILE}"
            VERBATIM
        )
    endforeach()

    add_custom_target(
        "${target_name}"
        ALL
        DEPENDS "${DST_HTML}" ${DST_DATA_FILES}
    )
endfunction()
