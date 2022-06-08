cmake_minimum_required(VERSION 2.8.12)

project(ui_taskbar)

set(SRCS)
if(WIN32)
    set(UI_TASKBAR_WIN 1)
    list(APPEND SRCS ui_taskbar_windows.cpp)
else()
    set(UI_TASKBAR_NULL 1)
    list(APPEND SRCS ui_taskbar_null.cpp)
endif()

configure_file(ui_taskbar_config.h.in ui_taskbar_config.h @ONLY)

add_library(ui_taskbar STATIC ${SRCS})

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        ui_taskbar PRIVATE -std=c++11 -Wall -Wextra -pedantic
    )
endif()

target_include_directories(
    ui_taskbar
    PRIVATE .
    PUBLIC .. "${CMAKE_CURRENT_BINARY_DIR}"
)

if(NOT TARGET dpso)
    add_subdirectory(../../dpso ${CMAKE_BINARY_DIR}/src/dpso)
endif()
target_link_libraries(ui_taskbar dpso)
