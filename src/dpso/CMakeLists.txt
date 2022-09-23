cmake_minimum_required(VERSION 2.8.12)

project(dpso)

option(
    DPSO_FORCE_TIMING
    "Force code timing (dpso::CodeTimer) even in release mode"
    NO)

set(BACKEND_SRCS)
if(UNIX AND NOT APPLE)
    list(
        APPEND BACKEND_SRCS
        backend/x11/x11_backend.cpp
        backend/x11/x11_key_manager.cpp
        backend/x11/x11_screenshot.cpp
        backend/x11/x11_selection.cpp)
elseif(WIN32)
    list(
        APPEND BACKEND_SRCS
        backend/windows/execution_layer/action_executor.cpp
        backend/windows/execution_layer/backend_executor.cpp
        backend/windows/execution_layer/key_manager_executor.cpp
        backend/windows/execution_layer/selection_executor.cpp
        backend/windows/utils/error.cpp
        backend/windows/windows_backend.cpp
        backend/windows/windows_key_manager.cpp
        backend/windows/windows_screenshot.cpp
        backend/windows/windows_selection.cpp)
else()
    message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} is not supported")
endif()

add_library(
    dpso

    delay.cpp
    dpso.cpp
    error.cpp
    geometry.cpp
    geometry_c.cpp
    hotkeys.cpp
    img.cpp
    key_names.cpp
    ocr.cpp
    ocr_engine/ocr_engine_creator.cpp
    ocr_engine/ocr_result.cpp
    ocr_engine/ocr_result_text.cpp
    ocr_engine/tesseract/lang_names.cpp
    ocr_engine/tesseract/tesseract_ocr_engine.cpp
    ocr_engine/tesseract/tesseract_ocr_engine_creator.cpp
    ocr_engine/tesseract/utils.cpp
    progress_tracker.cpp
    selection.cpp
    str.cpp

    ${BACKEND_SRCS})

target_compile_definitions(
    dpso
    PRIVATE
    DPSO_FORCE_TIMING=$<BOOL:${DPSO_FORCE_TIMING}>
    DPSO_DLL=$<BOOL:${BUILD_SHARED_LIBS}>)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        dpso PRIVATE -std=c++11 -Wall -Wextra -pedantic)
endif()

set_target_properties(
    dpso PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

target_include_directories(dpso PRIVATE . PUBLIC ..)

add_subdirectory(
    ../thirdparty/stb_image_resize
    "${CMAKE_CURRENT_BINARY_DIR}/src/thirdparty/stb_image_resize")
target_link_libraries(dpso stb_image_resize)

set(CMAKE_THREAD_PREFER_PTHREAD True)
find_package(Threads REQUIRED)
if(CMAKE_USE_PTHREADS_INIT)
    target_compile_options(dpso PRIVATE -pthread)
endif()
target_link_libraries(dpso ${CMAKE_THREAD_LIBS_INIT})

if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    if(NOT X11_Xshape_FOUND)
        message(SEND_ERROR "X11 Shape Extension is not found")
    endif()

    target_include_directories(
        dpso PRIVATE ${X11_INCLUDE_DIR} ${X11_Xshape_INCLUDE_PATH})
    target_link_libraries(dpso ${X11_LIBRARIES} ${X11_Xext_LIB})
endif()

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_search_module(TESSERACT tesseract)

    if(TESSERACT_FOUND)
        string(FIND "${TESSERACT_VERSION}" "." DOT_POS)
        string(
            SUBSTRING
            "${TESSERACT_VERSION}"
            0 ${DOT_POS}
            TESSERACT_VERSION_MAJOR)
    endif()
endif()

# TODO: If Tesseract was installed via CMake, we can also try to use
# find_package(Tesseract).
if(NOT TESSERACT_FOUND)
    # No pkg-config or an old Tesseract version that doesn't provide
    # a .pc file.
    find_path(
        TESSERACT_INCLUDE_DIRS baseapi.h PATH_SUFFIXES tesseract)
    if(NOT TESSERACT_INCLUDE_DIRS)
        message(SEND_ERROR "Can't find tesseract include dir")
    endif()

    find_library(TESSERACT_LIBRARIES tesseract)
    if(NOT TESSERACT_INCLUDE_DIRS)
        message(SEND_ERROR "Can't find tesseract library")
    endif()

    # Try to extract the major version from the header.
    find_file(
        TESSERACT_VERSION_SOURCE_H
        NAMES
        version.h # v5
        tess_version.h # v4
        baseapi.h # v3
        PATHS "${TESSERACT_INCLUDE_DIRS}"
        NO_DEFAULT_PATH)
    if(TESSERACT_VERSION_SOURCE_H)
        file(READ "${TESSERACT_VERSION_SOURCE_H}" VERSION_SOURCE_DATA)
        string(
            REGEX MATCH
            # Tesseract has TESSERACT_MAJOR_VERSION since version 4.
            # We use TESSERACT_VERSION_STR instead to support v3 as
            # well.
            "#define TESSERACT_VERSION_STR \"([0-9]*)."
            _
            "${VERSION_SOURCE_DATA}")
        set(TESSERACT_VERSION_MAJOR "${CMAKE_MATCH_1}")
    endif()

    mark_as_advanced(
        TESSERACT_INCLUDE_DIRS
        TESSERACT_LIBRARIES
        TESSERACT_VERSION_SOURCE_H)
endif()

set(DPSO_TESSERACT_VERSION_MAJOR
    "${TESSERACT_VERSION_MAJOR}"
    CACHE
    STRING
    "Major version of the found Tesseract library"
    FORCE)
mark_as_advanced(DPSO_TESSERACT_VERSION_MAJOR)

target_include_directories(dpso PRIVATE ${TESSERACT_INCLUDE_DIRS})
target_link_libraries(dpso ${TESSERACT_LIBRARIES})
