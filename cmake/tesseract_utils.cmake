
include(CMakeParseArguments)

# Get name of tesseract data directory to be used in install().
#
# The returned string may be empty if the Tesseract version was not
# detected.
function(get_tesseract_data_dir_name VAR)
    if(DPSO_TESSERACT_VERSION_MAJOR)
        set(${VAR}
            "tesseract_${DPSO_TESSERACT_VERSION_MAJOR}_data"
            PARENT_SCOPE)
    else()
        set(${VAR} "" PARENT_SCOPE)
    endif()
endfunction()

# Copy contents of tessdata directory.
#
# copy_tessdata(
#   SRC_DIR
#   DST_DIR
#   LANGUAGES languages...
#   OPTIONAL)
#
# LANGUAGES is a list of traineddata files without extensions.
# If OPTIONAL is given, nonexistent files are not treated as errors.
function(copy_tessdata SRC_DIR DST_DIR)
    cmake_parse_arguments(ARG "OPTIONAL" "" "LANGUAGES" ${ARGN})

    set(DST_FILES)

    foreach(LANG ${ARG_LANGUAGES})
        set(TRAINEDDATA_NAME "${LANG}.traineddata")
        set(SRC_FILE "${SRC_DIR}/${TRAINEDDATA_NAME}")
        set(DST_FILE "${DST_DIR}/${TRAINEDDATA_NAME}")

        if(NOT EXISTS "${SRC_FILE}")
            if(ARG_OPTIONAL)
                message("${SRC_FILE} does not exist")
                continue()
            endif()

            message(FATAL_ERROR "${SRC_FILE} does not exist")
        endif()

        add_custom_command(
            OUTPUT "${DST_FILE}"
            COMMAND
                "${CMAKE_COMMAND}" -E copy "${SRC_FILE}" "${DST_FILE}"
            DEPENDS "${SRC_FILE}"
            VERBATIM)

        list(APPEND DST_FILES "${DST_FILE}")
    endforeach()

    add_custom_target(tessdata_dir ALL DEPENDS ${DST_FILES})
endfunction()
