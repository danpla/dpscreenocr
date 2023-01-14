cmake_minimum_required(VERSION 3.16)

project(dpso_ext)

add_library(
    dpso_ext

    cfg.cpp
    cfg_ext.cpp
    history.cpp
    history_export.cpp
    str_nformat.cpp)

if(UNIX)
    target_sources(
        dpso_ext
        PRIVATE
        exec_unix.cpp)
endif()

if(UNIX AND NOT APPLE)
    target_sources(
        dpso_ext
        PRIVATE
        user_dirs_unix_xdg.cpp)
elseif(WIN32)
    target_sources(
        dpso_ext
        PRIVATE
        exec_windows.cpp
        user_dirs_windows.cpp
        windows_utils.cpp)
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

if(NOT TARGET dpso)
    add_subdirectory(../dpso "${CMAKE_BINARY_DIR}/src/dpso")
endif()

if(NOT TARGET dpso_utils)
    add_subdirectory(
        ../dpso_utils "${CMAKE_BINARY_DIR}/src/dpso_utils")
endif()


target_link_libraries(dpso_ext PUBLIC dpso PRIVATE dpso_utils)
