cmake_minimum_required(VERSION 2.8.12)

project(common)

if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)
    set(APP_LOCALE_DIR "${CMAKE_INSTALL_FULL_LOCALEDIR}")
else()
    set(APP_LOCALE_DIR "")
endif()

set(IN_SRCS app_info.cpp file_names.cpp paths.cpp)
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

    cfg_default_values.cpp
    cfg_keys.cpp
    ${CONFIGURED_SRCS}
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        common PRIVATE -std=c++11 -Wall -Wextra -pedantic
    )
endif()

target_include_directories(common PRIVATE . PUBLIC ..)

if(NOT TARGET dpso)
    add_subdirectory(../../dpso ${CMAKE_BINARY_DIR}/src/dpso)
endif()
target_link_libraries(common dpso)
