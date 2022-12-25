cmake_minimum_required(VERSION 3.16)

project(dpso)

option(
    DPSO_FORCE_TIMING
    "Force code timing (dpso::CodeTimer) even in release mode"
    NO)
mark_as_advanced(DPSO_FORCE_TIMING)

add_library(
    dpso

    delay.cpp
    dpso.cpp
    error.cpp
    geometry.cpp
    geometry_c.cpp
    img.cpp
    key_manager.cpp
    keys.cpp
    ocr.cpp
    ocr_engine/ocr_engine_creator.cpp
    ocr_engine/tesseract/lang_names.cpp
    ocr_engine/tesseract/tesseract_ocr_engine.cpp
    ocr_engine/tesseract/tesseract_ocr_engine_creator.cpp
    ocr_engine/tesseract/utils.cpp
    progress_tracker.cpp
    selection.cpp
    str.cpp
    strftime.cpp)

if(UNIX AND NOT APPLE)
    target_sources(
        dpso
        PRIVATE
        backend/x11/x11_backend.cpp
        backend/x11/x11_key_manager.cpp
        backend/x11/x11_screenshot.cpp
        backend/x11/x11_selection.cpp)
elseif(WIN32)
    target_sources(
        dpso
        PRIVATE
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

target_compile_definitions(
    dpso
    PRIVATE
    DPSO_FORCE_TIMING=$<BOOL:${DPSO_FORCE_TIMING}>
    DPSO_DLL=$<BOOL:${BUILD_SHARED_LIBS}>)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        dpso PRIVATE -std=c++17 -Wall -Wextra -pedantic)
endif()

set_target_properties(
    dpso PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

target_include_directories(dpso PRIVATE . PUBLIC ..)

add_subdirectory(
    ../thirdparty/stb_image_resize
    "${CMAKE_CURRENT_BINARY_DIR}/src/thirdparty/stb_image_resize")
target_link_libraries(dpso PRIVATE stb_image_resize)

find_package(Threads REQUIRED)
target_link_libraries(dpso PRIVATE ${CMAKE_THREAD_LIBS_INIT})

if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    if(NOT X11_Xshape_FOUND)
        message(SEND_ERROR "X11 Shape Extension is not found")
    endif()

    target_include_directories(
        dpso PRIVATE ${X11_INCLUDE_DIR} ${X11_Xshape_INCLUDE_PATH})
    target_link_libraries(dpso PRIVATE ${X11_LIBRARIES} ${X11_Xext_LIB})
endif()

find_package(PkgConfig REQUIRED)
if(PKG_CONFIG_FOUND)
    pkg_search_module(TESSERACT REQUIRED tesseract>=4.1.0)
endif()

# TODO: If Tesseract was installed via CMake, we can also try to use
# find_package(Tesseract). For now, we only build on various Linux
# distributions and MSYS2, all of which use pkg-config.

string(FIND "${TESSERACT_VERSION}" "." TMP_DOT_POS)
string(
    SUBSTRING
    "${TESSERACT_VERSION}"
    0
    ${TMP_DOT_POS}
    TMP_TESSERACT_VERSION_MAJOR)
unset(TMP_DOT_POS)

set(DPSO_TESSERACT_VERSION_MAJOR
    "${TMP_TESSERACT_VERSION_MAJOR}"
    CACHE
    STRING
    "Major version of the found Tesseract library"
    FORCE)
mark_as_advanced(DPSO_TESSERACT_VERSION_MAJOR)
unset(TMP_TESSERACT_VERSION_MAJOR)

target_include_directories(dpso PRIVATE ${TESSERACT_INCLUDE_DIRS})
target_link_libraries(dpso PRIVATE ${TESSERACT_LIBRARIES})
