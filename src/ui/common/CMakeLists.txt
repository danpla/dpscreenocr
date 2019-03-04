cmake_minimum_required(VERSION 2.8.12)

project(common)

if (UNIX AND NOT APPLE)
    include(GNUInstallDirs)
    set(APP_LOCALE_DIR "${CMAKE_INSTALL_FULL_LOCALEDIR}")
else()
    message(SEND_ERROR "Set APP_LOCALE_DIR for this platform")
endif()


set(IN_SRCS app_info.cpp paths.cpp)
set(CONFIGURED_SRCS "")
foreach(IN_SRC ${IN_SRCS})
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/${IN_SRC}.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${IN_SRC}"
        @ONLY
    )

    list(
        APPEND
        CONFIGURED_SRCS "${CMAKE_CURRENT_BINARY_DIR}/${IN_SRC}"
    )
endforeach()


add_library(
    common

    cfg_keys.cpp
    file_names.cpp
    ${CONFIGURED_SRCS}
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        common
        PRIVATE -std=c++11 -Wall -Wextra -pedantic
    )
endif()

set_target_properties(
    common
    PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

target_include_directories(common PRIVATE .)
target_include_directories(common PUBLIC ..)
