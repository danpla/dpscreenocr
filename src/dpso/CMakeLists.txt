cmake_minimum_required(VERSION 3.16)

project(dpso)

include(CMakeDependentOption)

string(
    JOIN " " DPSO_USE_DEFAULT_TESSERACT_DATA_PATH_DESCRIPTION
    "Use the default Tesseract data path instead of the explicit one."
    "You should disable this if Tesseract is going to be shipped"
    "with the program. Otherwise, the language manager will be"
    "disabled, because language packs for system-wide Tesseract"
    "should be installed via the package manager.")
cmake_dependent_option(
    DPSO_USE_DEFAULT_TESSERACT_DATA_PATH
    "${DPSO_USE_DEFAULT_TESSERACT_DATA_PATH_DESCRIPTION}"
    YES
    "UNIX; NOT APPLE"
    NO)

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
    ocr/lang_code_validator.cpp
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
        backend/x11/backend.cpp
        backend/x11/key_manager.cpp
        backend/x11/screenshot.cpp
        backend/x11/selection.cpp)
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

# Language manager
if(NOT DPSO_USE_DEFAULT_TESSERACT_DATA_PATH)
    target_sources(
        dpso
        PRIVATE
        ocr/remote_files_lang_manager.cpp
        ocr/tesseract/lang_manager.cpp)

    if(NOT TARGET dpso_json)
        add_subdirectory(
            ../dpso_json "${CMAKE_BINARY_DIR}/src/dpso_json")
    endif()

    if(NOT TARGET dpso_net)
        add_subdirectory(
            ../dpso_net "${CMAKE_BINARY_DIR}/src/dpso_net")
    endif()

    target_link_libraries(dpso PRIVATE dpso_json dpso_net)
else()
    target_sources(dpso PRIVATE ocr/tesseract/lang_manager_null.cpp)
endif()

target_compile_definitions(
    dpso
    PRIVATE
    DPSO_DLL=$<BOOL:${BUILD_SHARED_LIBS}>
    DPSO_USE_DEFAULT_TESSERACT_DATA_PATH=$<BOOL:${DPSO_USE_DEFAULT_TESSERACT_DATA_PATH}>)

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

if(NOT TARGET stb_image_resize)
    add_subdirectory(
        ../thirdparty/stb_image_resize
        "${CMAKE_BINARY_DIR}/src/thirdparty/stb_image_resize")
endif()

find_package(Threads REQUIRED)

target_link_libraries(
    dpso
    PRIVATE dpso_utils stb_image_resize ${CMAKE_THREAD_LIBS_INIT})

if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    if(NOT X11_Xshape_FOUND)
        message(SEND_ERROR "X11 Shape Extension is not found")
    endif()

    target_include_directories(
        dpso PRIVATE ${X11_INCLUDE_DIR} ${X11_Xshape_INCLUDE_PATH})
    target_link_libraries(
        dpso PRIVATE ${X11_LIBRARIES} ${X11_Xext_LIB})
endif()

find_package(PkgConfig REQUIRED)

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
