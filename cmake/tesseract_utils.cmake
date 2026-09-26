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

# Install contents of the tessdata directory.
#
# install_tessdata(
#   SRC_DIR
#   DST_DIR
#   LANGUAGES languages...
#   OPTIONAL)
#
# LANGUAGES is a list of traineddata files without extensions. If
# OPTIONAL is given, nonexistent files are not treated as errors.
function(install_tessdata SRC_DIR DST_DIR)
    cmake_parse_arguments(ARG "OPTIONAL" "" "LANGUAGES" ${ARGN})

    set(SRC_FILES)

    foreach(LANG ${ARG_LANGUAGES})
        set(SRC_FILE "${SRC_DIR}/${LANG}.traineddata")

        if(NOT EXISTS "${SRC_FILE}")
            if(ARG_OPTIONAL)
                message("${SRC_FILE} does not exist")
                continue()
            endif()

            message(FATAL_ERROR "${SRC_FILE} does not exist")
        endif()

        list(APPEND SRC_FILES "${SRC_FILE}")
    endforeach()

    install(FILES ${SRC_FILES} DESTINATION "${DST_DIR}")
endfunction()
