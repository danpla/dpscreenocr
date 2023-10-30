
find_program(PANDOC_EXE pandoc)
if(NOT PANDOC_EXE)
    message(SEND_ERROR "pandoc not found")
endif()

function(gen_html_manual DST_DIR)
    set(DOC_DIR "${CMAKE_SOURCE_DIR}/doc")
    set(DST_HTML "${DST_DIR}/manual.html")

    configure_file(
        "${DOC_DIR}/manual-metadata.yaml.in"
        "${CMAKE_BINARY_DIR}/manual-metadata.yaml"
        @ONLY)

    add_custom_command(
        OUTPUT "${DST_HTML}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DST_DIR}"
        COMMAND
            "${PANDOC_EXE}"
            --from=markdown
            --to=html5
            --eol=lf
            --standalone
            --css=manual-data/manual.css
            "--template=${DOC_DIR}/manual-data/template.html"
            "--output=${DST_HTML}"
            --toc
            --number-sections
            "${CMAKE_BINARY_DIR}/manual-metadata.yaml"
            "${DOC_DIR}/manual.md"
        DEPENDS
            "${DOC_DIR}/manual.md"
            "${CMAKE_BINARY_DIR}/manual-metadata.yaml"
            "${DOC_DIR}/manual-data/template.html"
        VERBATIM)

    file(
        GLOB
        SRC_DATA_FILES
        CONFIGURE_DEPENDS
        "${DOC_DIR}/manual-data/*.png"
        "${DOC_DIR}/manual-data/*.svg"
        "${DOC_DIR}/manual-data/*.css")

    set(DST_DATA_FILES)
    foreach(SRC_FILE ${SRC_DATA_FILES})
        get_filename_component(SRC_FILE_NAME "${SRC_FILE}" NAME)
        set(DST_FILE "${DST_DIR}/manual-data/${SRC_FILE_NAME}")
        list(APPEND DST_DATA_FILES "${DST_FILE}")

        add_custom_command(
            OUTPUT "${DST_FILE}"
            COMMAND
                "${CMAKE_COMMAND}" -E copy "${SRC_FILE}" "${DST_FILE}"
            DEPENDS "${SRC_FILE}"
            VERBATIM)
    endforeach()

    add_custom_target(
        html_manual ALL DEPENDS "${DST_HTML}" ${DST_DATA_FILES})
endfunction()
