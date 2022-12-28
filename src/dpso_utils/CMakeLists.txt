cmake_minimum_required(VERSION 3.16)

project(dpso_utils)

add_library(
    dpso_utils

    cfg.cpp
    cfg_ext.cpp
    history.cpp
    history_export.cpp
    os.cpp
    str_format.cpp)

if(UNIX)
    target_sources(
        dpso_utils
        PRIVATE
        exec_posix.cpp
        os_unix.cpp
        unix_utils.cpp)
endif()

if(UNIX AND NOT APPLE)
    target_sources(
        dpso_utils
        PRIVATE
        user_dirs_unix_xdg.cpp)
elseif(WIN32)
    target_sources(
        dpso_utils
        PRIVATE
        exec_windows.cpp
        os_windows.cpp
        user_dirs_windows.cpp
        windows_utils.cpp)
else()
    message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} is not supported")
endif()

target_compile_features(dpso_utils PRIVATE cxx_std_17)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(dpso_utils PRIVATE -Wall -Wextra -pedantic)
endif()

set_target_properties(
    dpso_utils
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

target_include_directories(dpso_utils PRIVATE . PUBLIC ..)

if(NOT TARGET dpso)
    add_subdirectory(../dpso "${CMAKE_BINARY_DIR}/src/dpso")
endif()
target_link_libraries(dpso_utils PUBLIC dpso)
