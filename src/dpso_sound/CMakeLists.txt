cmake_minimum_required(VERSION 3.16)

project(dpso_sound)

add_library(dpso_sound)

if(WIN32)
    target_sources(dpso_sound PRIVATE sound_windows.cpp)
else()
    target_sources(dpso_sound PRIVATE sound_null.cpp)
endif()

set_target_properties(
    dpso_sound PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(dpso_sound PRIVATE -Wall -Wextra -pedantic)
endif()

if(NOT TARGET dpso_utils)
    add_subdirectory(
        ../dpso_utils "${CMAKE_BINARY_DIR}/src/dpso_utils")
endif()

target_include_directories(dpso_sound PUBLIC .. PRIVATE .)

