# Install icons from "data/icons/sizes" to DST_DIR.
#
# install_icons(
#   DST_DIR
#   RASTER_SIZES sizes...
#   INCLUDE_SCALABLE)
#
# RASTER_SIZES is either a list of raster icon sizes to include, or
# "all" to include all sizes. INCLUDE_SCALABLE includes SVG icons.
function(install_icons DST_DIR)
    cmake_parse_arguments(
        ARG "INCLUDE_SCALABLE" "" "RASTER_SIZES" ${ARGN})

    set(SCALABLE_SIZE "scalable")

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

    if(INCLUDE_ALL_SIZES)
        file(
            GLOB
            SIZES
            RELATIVE "${SRC_DIR}"
            CONFIGURE_DEPENDS
            "${SRC_DIR}/*")
        list(FILTER SIZES INCLUDE REGEX "^([0-9]+)$")
    else()
        set(SIZES ${ARG_RASTER_SIZES})
    endif()

    if(ARG_INCLUDE_SCALABLE
            AND IS_DIRECTORY "${SRC_DIR}/${SCALABLE_SIZE}")
        list(APPEND SIZES "${SCALABLE_SIZE}")
    endif()

    if(NOT SIZES)
        return()
    endif()

    list(SORT SIZES)

    foreach(SIZE ${SIZES})
        if(SIZE STREQUAL SCALABLE_SIZE)
            set(EXT ".svg")
        else()
            set(EXT ".png")
        endif()

        file(
            GLOB
            SRC_FILES
            CONFIGURE_DEPENDS
            "${SRC_DIR}/${SIZE}/*${EXT}")

        install(FILES ${SRC_FILES} DESTINATION "${DST_DIR}/${SIZE}")
    endforeach()
endfunction()
