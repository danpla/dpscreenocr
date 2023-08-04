cmake_minimum_required(VERSION 3.16)

project(dpso)

add_library(
    dpso

    dpso.cpp
    geometry.cpp
    geometry_c.cpp
    img.cpp
    key_manager.cpp
    keys.cpp
    ocr.cpp
    ocr/engine.cpp
    ocr/tesseract/engine.cpp
    ocr/tesseract/lang_names.cpp
    ocr/tesseract/lang_utils.cpp
    ocr/tesseract/recognizer.cpp
    ocr/tesseract/utils.cpp
    ocr_data_lock.cpp
    ocr_engine.cpp
    ocr_lang_manager.cpp
    selection.cpp)

# Backend
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
        backend/windows/windows_backend.cpp
        backend/windows/windows_key_manager.cpp
        backend/windows/windows_screenshot.cpp
        backend/windows/windows_selection.cpp)
else()
    message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} is not supported")
endif()

find_package(PkgConfig REQUIRED)

# Language manager
# For now, the language manager is only available on Windows, because
# on Unix-like systems we use the system package manager instead. But
# in fact, dpso_net is the only piece that is not implemented for
# Unix-like systems, so we can remove platform-specific checks below
# once we add a libcurl backed for dpso_net.
if(WIN32)
    target_sources(
        dpso
        PRIVATE
        ocr/remote_files_lang_manager.cpp
        ocr/tesseract/lang_manager.cpp
        ocr/tesseract/lang_manager_error_utils.cpp)

    if(NOT TARGET dpso_net)
        add_subdirectory(
            ../dpso_net "${CMAKE_BINARY_DIR}/src/dpso_net")
    endif()
    target_link_libraries(dpso PRIVATE dpso_net)

    pkg_search_module(JANSSON REQUIRED jansson>=2)
    target_include_directories(dpso PRIVATE ${JANSSON_INCLUDE_DIRS})
    target_link_libraries(dpso PRIVATE ${JANSSON_LIBRARIES})
else()
    target_sources(dpso PRIVATE ocr/tesseract/lang_manager_null.cpp)
endif()

target_compile_definitions(
    dpso PRIVATE DPSO_DLL=$<BOOL:${BUILD_SHARED_LIBS}>)

set_target_properties(
    dpso PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(dpso PRIVATE -Wall -Wextra -pedantic)
endif()

target_include_directories(dpso PRIVATE . PUBLIC ..)

if(NOT TARGET dpso_utils)
    add_subdirectory(
        ../dpso_utils "${CMAKE_BINARY_DIR}/src/dpso_utils")
endif()
target_link_libraries(dpso PRIVATE dpso_utils)

if(NOT TARGET stb_image_resize)
    add_subdirectory(
        ../thirdparty/stb_image_resize
        "${CMAKE_BINARY_DIR}/src/thirdparty/stb_image_resize")
endif()
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

# TODO: If Tesseract was installed via CMake, we can also try to use
# find_package(Tesseract). For now, we only build on various Linux
# distributions and MSYS2, all of which use pkg-config.
pkg_search_module(TESSERACT REQUIRED tesseract>=4.1.0)

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
