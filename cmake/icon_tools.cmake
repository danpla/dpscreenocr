
find_program(RSVG_CONVERT_EXE rsvg-convert)
if(NOT RSVG_CONVERT_EXE)
    message(FATAL_ERROR "rsvg-convert not found")
endif()

find_program(OPTIPNG_EXE optipng)
if(NOT OPTIPNG_EXE)
    message(FATAL_ERROR "optipng not found")
endif()

set(SVG_TO_PNG "${CMAKE_BINARY_DIR}/svg_to_png.cmake")

configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/icon_tools_svg_to_png.cmake.in"
    "${SVG_TO_PNG}"
    @ONLY)

function(get_svg_path ICON_NAME SIZE OUT_PATH)
    set(SVG_NAME "${ICON_NAME}.svg")

    set(SIZES_DIR "${CMAKE_SOURCE_DIR}/data/icons/sizes")

    set(SVG_PATH "${SIZES_DIR}/scalable/${SVG_NAME}")

    # The generic SVG file must exist even if there's an SVG for a
    # specific size.
    if(NOT EXISTS "${SVG_PATH}")
        message(FATAL_ERROR "\"${SVG_PATH}\" does not exist")
    endif()

    set(SVG_SIZE_PATH "${SIZES_DIR}/${SIZE}/${SVG_NAME}")
    if(EXISTS "${SVG_SIZE_PATH}")
        set(SVG_PATH "${SVG_SIZE_PATH}")
    endif()

    set(${OUT_PATH} ${SVG_PATH} PARENT_SCOPE)
endfunction()

function(add_svg_to_png_command ICON_NAME PNG_PATH SIZE)
    get_svg_path("${ICON_NAME}" "${SIZE}" SVG_PATH)

    add_custom_command(
        OUTPUT "${PNG_PATH}"
        COMMAND
            "${CMAKE_COMMAND}"
            -D "SVG_PATH=${SVG_PATH}"
            -D "PNG_PATH=${PNG_PATH}"
            -D "SIZE=${SIZE}"
            -P "${SVG_TO_PNG}"
        DEPENDS "${SVG_PATH}"
        VERBATIM)
endfunction()

# Generate icons.
#
# gen_icons(
#   DST_DIR
#   RASTER_SIZES sizes...
#   INCLUDE_SCALABLE)
#
# RASTER_SIZES is a list of sizes for generated PNGs. Each PNG will be
# placed in a "${DST_DIR}/${SIZE}" directory. INCLUDE_SCALABLE copies
# SVGs to the "${DST_DIR}/scalable" directory.
function(gen_icons DST_DIR)
    cmake_parse_arguments(
        ARG "INCLUDE_SCALABLE" "" "RASTER_SIZES" ${ARGN})

    set(SIZES_DIR "${CMAKE_SOURCE_DIR}/data/icons/sizes")

    file(
        GLOB
        ICON_NAMES
        RELATIVE "${SIZES_DIR}/scalable"
        CONFIGURE_DEPENDS
        "${SIZES_DIR}/scalable/*.svg")
    list(TRANSFORM ICON_NAMES REPLACE ".svg$" "")
    list(SORT ICON_NAMES)

    set(DST_FILES)

    foreach(ICON_NAME ${ICON_NAMES})
        foreach(SIZE ${ARG_RASTER_SIZES})
            set(PNG_PATH "${DST_DIR}/${SIZE}/${ICON_NAME}.png")

            add_svg_to_png_command(
                "${ICON_NAME}" "${PNG_PATH}" "${SIZE}")

            list(APPEND DST_FILES "${PNG_PATH}")
        endforeach()
    endforeach()

    if(ARG_INCLUDE_SCALABLE)
        foreach(ICON_NAME ${ICON_NAMES})
            set(SRC_PATH "${SIZES_DIR}/scalable/${ICON_NAME}.svg")
            set(DST_PATH "${DST_DIR}/scalable/${ICON_NAME}.svg")

            add_custom_command(
                OUTPUT "${DST_PATH}"
                COMMAND
                    "${CMAKE_COMMAND}" -E copy
                    "${SRC_PATH}" "${DST_PATH}"
                DEPENDS "${SRC_PATH}"
                VERBATIM)

            list(APPEND DST_FILES "${DST_PATH}")
        endforeach()
    endif()

    add_custom_target(icons ALL DEPENDS ${DST_FILES})
endfunction()

# Generate ${APP_FILE_NAME}.ico in DST_DIR.
function(gen_windows_ico DST_DIR)
    find_program(ICOTOOL_EXE icotool)
    if(NOT ICOTOOL_EXE)
        message(FATAL_ERROR "icotool not found")
    endif()

    set(ICON_NAME "${APP_FILE_NAME}")

    set(PNGS_DIR "${CMAKE_BINARY_DIR}/windows-ico-pngs")

    # PNG paths for icotool, with and without "--raw".
    set(RAW_PNGS)
    set(NON_RAW_PNGS)

    # See:
    # https://learn.microsoft.com/en-us/windows/win32/uxguide/vis-icons#size-requirements
    set(APP_ICON_SIZES 16 32 48 256)
    foreach(SIZE ${APP_ICON_SIZES})
        set(PNG_PATH "${PNGS_DIR}/${ICON_NAME}-${SIZE}.png")

        add_svg_to_png_command("${ICON_NAME}" "${PNG_PATH}" "${SIZE}")

        list(APPEND PNG_PATHS "${PNG_PATH}")

        if(SIZE GREATER_EQUAL 64)
            list(APPEND RAW_PNGS "--raw=${PNG_PATH}")
        else()
            list(APPEND NON_RAW_PNGS "${PNG_PATH}")
        endif()
    endforeach()

    set(DST_ICO "${DST_DIR}/${ICON_NAME}.ico")

    add_custom_command(
        OUTPUT "${DST_ICO}"
        COMMAND
            "${ICOTOOL_EXE}" --create "--output=${DST_ICO}"
            ${RAW_PNGS} ${NON_RAW_PNGS}
        DEPENDS ${PNG_PATHS}
        VERBATIM)

    add_custom_target("${ICON_NAME}_ico" ALL DEPENDS "${DST_ICO}")
endfunction()
