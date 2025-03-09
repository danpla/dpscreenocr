cmake_minimum_required(VERSION 3.22)

project(dpso_ext)

add_library(
    dpso_ext

    cfg.cpp
    cfg_ext.cpp
    history.cpp
    history_export.cpp)

if(UNIX AND NOT APPLE)
    target_sources(
        dpso_ext
        PRIVATE
        user_dirs_unix_xdg.cpp)
elseif(WIN32)
    target_sources(
        dpso_ext
        PRIVATE
        user_dirs_windows.cpp)
else()
    message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} is not supported")
endif()

set_target_properties(
    dpso_ext PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(dpso_ext PRIVATE -Wall -Wextra -pedantic)
endif()

target_include_directories(dpso_ext PRIVATE . PUBLIC ..)

if(NOT TARGET dpso_ocr)
    add_subdirectory(../dpso_ocr "${CMAKE_BINARY_DIR}/src/dpso_ocr")
endif()

if(NOT TARGET dpso_sys)
    add_subdirectory(../dpso_sys "${CMAKE_BINARY_DIR}/src/dpso_sys")
endif()

if(NOT TARGET dpso_utils)
    add_subdirectory(
        ../dpso_utils "${CMAKE_BINARY_DIR}/src/dpso_utils")
endif()

target_link_libraries(
    dpso_ext PUBLIC dpso_ocr dpso_sys PRIVATE dpso_utils)
