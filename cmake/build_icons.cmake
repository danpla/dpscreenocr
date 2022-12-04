
include(CMakeParseArguments)

# Copy icons from "data/icons/sizes" to dst_dir.
#
# build_icons(
#   dst_dir
#   RASTER_SIZES sizes...
#   INCLUDE_SCALABLE)
#
# RASTER_SIZES is either a list of raster icon sizes to include, or
# "all" to include all sizes. INCLUDE_SCALABLE includes SVG icons.
function(build_icons dst_dir)
    cmake_parse_arguments(
        ARG "INCLUDE_SCALABLE" "" "RASTER_SIZES" ${ARGN})

    set(SRC_DIR "${CMAKE_SOURCE_DIR}/data/icons/sizes")

    set(INCLUDE_ALL_SIZES FALSE)
    foreach(SIZE ${ARG_RASTER_SIZES})
        if(SIZE STREQUAL "all")
            set(INCLUDE_ALL_SIZES TRUE)
            break()
        elseif(NOT IS_DIRECTORY "${SRC_DIR}/${SIZE}")
            message(
                FATAL_ERROR "No ${SIZE} icon size in \"${SRC_DIR}\"")
        endif()
    endforeach()

    set(SIZES)
    if(INCLUDE_ALL_SIZES)
        file(
            GLOB
            SIZE_DIRS
            RELATIVE "${SRC_DIR}"
            CONFIGURE_DEPENDS
            "${SRC_DIR}/*")
        foreach(DIR ${SIZE_DIRS})
            string(REGEX MATCH "^([0-9]+)$" _ "${DIR}")
            if (CMAKE_MATCH_1)
                list(APPEND SIZES "${CMAKE_MATCH_1}")
            endif()
        endforeach()
    else()
        list(APPEND SIZES ${ARG_RASTER_SIZES})
    endif()

    if(ARG_INCLUDE_SCALABLE AND IS_DIRECTORY "${SRC_DIR}/scalable")
        list(APPEND SIZES "scalable")
    endif()

    if(NOT SIZES)
        return()
    endif()

    list(SORT SIZES)

    set(DST_FILES)
    foreach(SIZE ${SIZES})
        file(GLOB SRC_FILES CONFIGURE_DEPENDS "${SRC_DIR}/${SIZE}/*")
        list(SORT SRC_FILES)

        foreach(SRC_FILE ${SRC_FILES})
            get_filename_component(SRC_FILE_NAME "${SRC_FILE}" NAME)
            set(DST_FILE "${dst_dir}/${SIZE}/${SRC_FILE_NAME}")

            add_custom_command(
                OUTPUT "${DST_FILE}"
                COMMAND
                    "${CMAKE_COMMAND}" -E copy
                    "${SRC_FILE}" "${DST_FILE}"
                DEPENDS "${SRC_FILE}"
                VERBATIM)

            list(APPEND DST_FILES "${DST_FILE}")
        endforeach()
    endforeach()

    add_custom_target(icons ALL DEPENDS ${DST_FILES})
endfunction()
