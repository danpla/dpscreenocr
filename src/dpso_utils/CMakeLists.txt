cmake_minimum_required(VERSION 3.16)

project(dpso_utils)

option(
    DPSO_FORCE_TIMING
    "Force code timing (dpso::CodeTimer) even in release mode"
    NO)
mark_as_advanced(DPSO_FORCE_TIMING)

add_library(
    dpso_utils

    delay.cpp
    error.cpp
    os.cpp
    progress_tracker.cpp
    str.cpp
    strftime.cpp)

if(UNIX)
    target_sources(
        dpso_utils
        PRIVATE
        os_unix.cpp
        unix/fsync.cpp
        unix/make_dirs.cpp)
elseif(WIN32)
    target_sources(
        dpso_utils
        PRIVATE
        os_windows.cpp
        windows/cmdline.cpp
        windows/error.cpp
        windows/utf.cpp)
else()
    message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} is not supported")
endif()

set_target_properties(
    dpso_utils PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(dpso_utils PRIVATE -Wall -Wextra -pedantic)
endif()

target_include_directories(dpso_utils PRIVATE . PUBLIC ..)

target_compile_definitions(
    dpso PUBLIC DPSO_FORCE_TIMING=$<BOOL:${DPSO_FORCE_TIMING}>)
