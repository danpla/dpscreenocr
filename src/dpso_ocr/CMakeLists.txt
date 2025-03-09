cmake_minimum_required(VERSION 3.22)

project(dpso_ocr)

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
    dpso_ocr

    data_lock.cpp
    engine.cpp
    engine/engine.cpp
    engine/lang_code_validator.cpp
    engine/tesseract/engine.cpp
    engine/tesseract/lang_names.cpp
    engine/tesseract/lang_utils.cpp
    engine/tesseract/recognizer.cpp
    engine/tesseract/utils.cpp
    lang_manager.cpp
    ocr.cpp)

# Language manager
if(NOT DPSO_USE_DEFAULT_TESSERACT_DATA_PATH)
    target_sources(
        dpso_ocr
        PRIVATE
        engine/remote_files_lang_manager.cpp
        engine/tesseract/lang_manager.cpp)

    if(NOT TARGET dpso_json)
        add_subdirectory(
            ../dpso_json "${CMAKE_BINARY_DIR}/src/dpso_json")
    endif()

    if(NOT TARGET dpso_net)
        add_subdirectory(
            ../dpso_net "${CMAKE_BINARY_DIR}/src/dpso_net")
    endif()

    target_link_libraries(dpso_ocr PRIVATE dpso_json dpso_net)
else()
    target_sources(
        dpso_ocr PRIVATE engine/tesseract/lang_manager_null.cpp)
endif()

target_compile_definitions(
    dpso_ocr
    PRIVATE
    DPSO_USE_DEFAULT_TESSERACT_DATA_PATH=$<BOOL:${DPSO_USE_DEFAULT_TESSERACT_DATA_PATH}>)

set_target_properties(
    dpso_ocr PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(dpso_ocr PRIVATE -Wall -Wextra -pedantic)
endif()

target_include_directories(dpso_ocr PRIVATE . PUBLIC ..)

if(NOT TARGET dpso_img)
    add_subdirectory(
        ../dpso_img "${CMAKE_BINARY_DIR}/src/dpso_img")
endif()

if(NOT TARGET dpso_utils)
    add_subdirectory(
        ../dpso_utils "${CMAKE_BINARY_DIR}/src/dpso_utils")
endif()

find_package(Threads REQUIRED)

target_link_libraries(
    dpso_ocr
    PUBLIC dpso_img
    PRIVATE dpso_utils ${CMAKE_THREAD_LIBS_INIT})

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

target_include_directories(dpso_ocr PRIVATE ${TESSERACT_INCLUDE_DIRS})
target_link_libraries(dpso_ocr PRIVATE ${TESSERACT_LIBRARIES})
