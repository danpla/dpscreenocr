cmake_minimum_required(VERSION 3.16)

project(dpso_sound)

add_library(dpso_sound)

if(UNIX AND NOT APPLE)
    target_sources(
        dpso_sound
        PRIVATE
        unix/lib_pulse.cpp
        unix/lib_sndfile.cpp
        unix/sound_pulse.cpp)

    find_package(Threads REQUIRED)

    target_link_libraries(
        dpso_sound PRIVATE dl ${CMAKE_THREAD_LIBS_INIT})
elseif(WIN32)
    target_sources(dpso_sound PRIVATE sound_windows.cpp)
    target_link_libraries(dpso_sound PRIVATE winmm)
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

include(TestBigEndian)
test_big_endian(IS_BIG_ENDIAN)

target_compile_definitions(
    dpso_sound
    PRIVATE DPSO_SOUND_IS_BIG_ENDIAN=$<BOOL:${IS_BIG_ENDIAN}>)

if(NOT TARGET dpso_utils)
    add_subdirectory(
        ../dpso_utils "${CMAKE_BINARY_DIR}/src/dpso_utils")
endif()

target_include_directories(dpso_sound PUBLIC .. PRIVATE .)
