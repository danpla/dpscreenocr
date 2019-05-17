cmake_minimum_required(VERSION 2.8.12)

project(dpso_utils)

option(DPSO_ENABLE_NLS "Enable gettext support" YES)

set(PLATFORM_SRCS)

if (UNIX)
    list(
        APPEND PLATFORM_SRCS
        exec_posix.cpp
        os_unix.cpp
    )
endif()

if (UNIX AND NOT APPLE)
    list(
        APPEND PLATFORM_SRCS
        cfg_path_unix_xdg.cpp
    )
elseif(WIN32)
    list(
        APPEND PLATFORM_SRCS
        cfg_path_windows.cpp
        exec_null.cpp
        os_windows.cpp
        windows_utils.cpp
    )
else()
    message(
        FATAL_ERROR
        "The current system (${CMAKE_SYSTEM_NAME}) is not supported"
    )
endif()

add_library(
    dpso_utils

    cfg.cpp
    cfg_ext.cpp
    history.cpp
    history_export.cpp
    intl.cpp
    lang_name.cpp
    str_format.cpp

    ${PLATFORM_SRCS}
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
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

target_include_directories(dpso_utils PRIVATE .)
target_include_directories(dpso_utils PUBLIC ..)

add_dependencies(dpso_utils dpso)
target_link_libraries(dpso_utils dpso)

target_compile_definitions(
    dpso_utils
    PUBLIC
    ENABLE_NLS=$<BOOL:${DPSO_ENABLE_NLS}>
)
if (DPSO_ENABLE_NLS)
    find_package(Intl REQUIRED)
    target_include_directories(
        dpso_utils
        PUBLIC
        ${Intl_INCLUDE_DIRS}
    )
    target_link_libraries(
        dpso_utils
        ${Intl_LIBRARIES}
    )
endif()
