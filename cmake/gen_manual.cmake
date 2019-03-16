
find_program(PANDOC_EXE pandoc)
if (NOT PANDOC_EXE)
    message(SEND_ERROR "pandoc not found")
endif()


function(gen_html_manual target_name build_dir)
    set(DOC_DIR "${CMAKE_SOURCE_DIR}/doc")

    add_custom_command(
        OUTPUT "${build_dir}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${build_dir}"
        VERBATIM
    )

    set(MANUAL_BUILD_HTML "${build_dir}/manual.html")
    separate_arguments(
        PANDOC_ARGS
        UNIX_COMMAND
        "--to=html5 --standalone --css=manual-data/manual.css --output=${MANUAL_BUILD_HTML} --template=${DOC_DIR}/manual-data/template.html --toc --number-sections -V \"title:dpScreenOCR Manual\" -V \"toctitle:Table of contents\" -V \"pagetitle:dpScreenOCR Manual\" --from=markdown ${DOC_DIR}/manual.md"
    )

    add_custom_command(
        OUTPUT "${MANUAL_BUILD_HTML}"
        COMMAND "${PANDOC_EXE}" ${PANDOC_ARGS}
        DEPENDS "${build_dir}" "${DOC_DIR}/manual.md" "${DOC_DIR}/manual-data/manual.css" "${DOC_DIR}/manual-data/template.html"
        VERBATIM
    )

    set(MANUAL_DATA_BUILD_DIR "${build_dir}/manual-data")

    add_custom_command(
        OUTPUT "${MANUAL_DATA_BUILD_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${MANUAL_DATA_BUILD_DIR}"
        DEPENDS "${build_dir}"
        VERBATIM
    )

    file(
        GLOB
        MANUAL_DATA_FILES
        "${DOC_DIR}/manual-data/*.png"
        "${DOC_DIR}/manual-data/*css"
    )

    set(MANUAL_BUILD_DATA)
    foreach(DATA_FILE ${MANUAL_DATA_FILES})
        get_filename_component(DATA_FILE_NAME "${DATA_FILE}" NAME)
        set(DATA_BUILD_FILE "${MANUAL_DATA_BUILD_DIR}/${DATA_FILE_NAME}")
        list(APPEND MANUAL_BUILD_DATA "${DATA_BUILD_FILE}")

        add_custom_command(
            OUTPUT "${DATA_BUILD_FILE}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DATA_FILE}" "${DATA_BUILD_FILE}"
            DEPENDS "${MANUAL_DATA_BUILD_DIR}" "${DATA_FILE}"
            VERBATIM
        )
    endforeach()

    add_custom_target(
        "${target_name}"
        ALL
        DEPENDS "${MANUAL_BUILD_HTML}" ${MANUAL_BUILD_DATA}
    )
endfunction()
