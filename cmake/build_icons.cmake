
include(CMakeParseArguments)

# The function copies icons from "/data/icons/hicolor" so that each
# icon is placed in "{dst_dir}/{size}" dir, where {size} is either a
# size of one side of a raster icon (icons are square), or "scalable"
# in case of SVG.
#
# build_icons(
#   dst_dir
#   RASTER_SIZES sizes...
#   INCLUDE_SCALABLE)
#
# RASTER_SIZES is either a list of raster icon sizes to include, or
# "all" to include all sizes. INCLUDE_SCALABLE includes SVG icons.
function(build_icons dst_dir)
    cmake_parse_arguments(ARG "INCLUDE_SCALABLE" "" "RASTER_SIZES" ${ARGN})

    set(SRC_DIR "${CMAKE_SOURCE_DIR}/data/icons/hicolor")

    set(INCLUDE_ALL_SIZES FALSE)
    foreach(SIZE ${ARG_RASTER_SIZES})
        if(SIZE STREQUAL "all")
            set(INCLUDE_ALL_SIZES TRUE)
            break()
        elseif(NOT IS_DIRECTORY "${SRC_DIR}/${SIZE}x${SIZE}")
            message(FATAL_ERROR "No ${SIZE} icon size in \"${SRC_DIR}\"")
        endif()
    endforeach()

    set(SIZES)
    if(INCLUDE_ALL_SIZES)
        file(GLOB SIZE_DIRS RELATIVE "${SRC_DIR}" "${SRC_DIR}/*")
        foreach(DIR ${SIZE_DIRS})
            string(REGEX MATCH "^([0-9]+)x([0-9]+)$" _ "${DIR}")
            if (CMAKE_MATCH_1 EQUAL CMAKE_MATCH_2)
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

    set(TARGET_DEPENDENCIES)

    list(SORT SIZES)
    foreach(SIZE ${SIZES})
        set(SIZE_DIR)
        if(SIZE STREQUAL "scalable")
            set(SIZE_DIR "scalable")
        else()
            set(SIZE_DIR "${SIZE}x${SIZE}")
        endif()

        file(GLOB_RECURSE SRC_FILES "${SRC_DIR}/${SIZE_DIR}/*")

        set(DST_SUBDIR "${dst_dir}/${SIZE}")
        list(APPEND TARGET_DEPENDENCIES "${DST_SUBDIR}")

        add_custom_command(
            OUTPUT "${DST_SUBDIR}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${DST_SUBDIR}"
            COMMAND ${CMAKE_COMMAND} -E copy ${SRC_FILES} "${DST_SUBDIR}"
            DEPENDS ${SRC_FILES}
            VERBATIM
        )
    endforeach()

    add_custom_target("icons" ALL DEPENDS ${TARGET_DEPENDENCIES})
endfunction()
