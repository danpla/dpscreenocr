cmake_minimum_required(VERSION 2.8.12)

project(ui_common)

set(
    SRCS
    cfg_default_values.cpp
    cfg_keys.cpp
    init_intl.cpp
    ocr_data_dir_name.cpp
)

# Files for configure_file()
set(IN_SRCS app_info.cpp file_names.cpp)

if(UNIX AND NOT APPLE)
    list(APPEND SRCS startup_setup_null.cpp dirs_unix.cpp)
    list(APPEND IN_SRCS dirs_unix_cfg.h)
elseif(WIN32)
    list(
        APPEND SRCS
        dirs_windows.cpp
        startup_setup_windows.cpp
    )
else()
    message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} is not supported")
endif()

foreach(IN_SRC ${IN_SRCS})
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/${IN_SRC}.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${IN_SRC}"
        @ONLY
    )

    list(APPEND SRCS "${CMAKE_CURRENT_BINARY_DIR}/${IN_SRC}")
endforeach()

add_library(ui_common STATIC ${SRCS})

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        ui_common PRIVATE -std=c++11 -Wall -Wextra -pedantic
    )
endif()

target_include_directories(ui_common PRIVATE . PUBLIC ..)

if(NOT TARGET dpso)
    add_subdirectory(../../dpso ${CMAKE_BINARY_DIR}/src/dpso)
endif()

if(NOT TARGET dpso_intl)
    add_subdirectory(
        ../../dpso_intl ${CMAKE_BINARY_DIR}/src/dpso_intl
    )
endif()

if(NOT TARGET dpso_utils)
    add_subdirectory(
        ../../dpso_utils ${CMAKE_BINARY_DIR}/src/dpso_utils
    )
endif()

target_link_libraries(ui_common dpso dpso_intl dpso_utils)
