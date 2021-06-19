cmake_minimum_required(VERSION 2.8.12)

project(dpso_utils)

set(PLATFORM_SRCS)

if(UNIX)
    list(
        APPEND PLATFORM_SRCS
        exec_posix.cpp
        os_unix.cpp
        unix_utils.cpp
    )
endif()

if(UNIX AND NOT APPLE)
    list(
        APPEND PLATFORM_SRCS
        cfg_path_unix_xdg.cpp
    )
elseif(WIN32)
    list(
        APPEND PLATFORM_SRCS
        cfg_path_windows.cpp
        exec_windows.cpp
        os_windows.cpp
        windows_utils.cpp
    )
else()
    message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} is not supported")
endif()

add_library(
    dpso_utils

    cfg.cpp
    cfg_ext.cpp
    cfg_path.cpp
    history.cpp
    history_export.cpp
    str_format.cpp

    ${PLATFORM_SRCS}
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        dpso_utils
        PRIVATE -std=c++11 -Wall -Wextra -pedantic
    )
endif()

set_target_properties(
    dpso_utils
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

target_include_directories(dpso_utils PRIVATE . PUBLIC ..)
target_link_libraries(dpso_utils dpso)
