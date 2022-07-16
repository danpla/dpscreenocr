
include(CMakeParseArguments)

# Get name of tesseract data directory to be used in install().
#
# The returned string may be empty if the Tesseract version was not
# detected.
function(get_tesseract_data_dir_name var)
    if(DPSO_TESSERACT_VERSION_MAJOR)
        set(
            ${var}
            "tesseract_${DPSO_TESSERACT_VERSION_MAJOR}_data"
            PARENT_SCOPE)
    else()
        set(${var} "" PARENT_SCOPE)
    endif()
endfunction()

# Copy contents of tessdata directory.
#
# copy_tessdata(
#   src_dir
#   dst_dir
#   LANGUAGES languages...
#   OPTIONAL)
#
# LANGUAGES is a list of traineddata files without extensions.
# If OPTIONAL is given, nonexistent files are not treated as errors.
function(copy_tessdata src_dir dst_dir)
    cmake_parse_arguments(ARG "OPTIONAL" "" "LANGUAGES" ${ARGN})

    set(TARGET_DEPENDENCIES)

    foreach(LANG ${ARG_LANGUAGES})
        set(TRAINEDDATA_NAME "${LANG}.traineddata")
        set(SRC_FILE "${src_dir}/${TRAINEDDATA_NAME}")
        set(DST_FILE "${dst_dir}/${TRAINEDDATA_NAME}")

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

        list(APPEND TARGET_DEPENDENCIES "${DST_FILE}")
    endforeach()

    add_custom_target(tessdata_dir ALL DEPENDS ${TARGET_DEPENDENCIES})
endfunction()
