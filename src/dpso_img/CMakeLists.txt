cmake_minimum_required(VERSION 3.16)

project(dpso_img)

add_library(dpso_img img.cpp ops.cpp pnm.cpp px_format.cpp)

set_target_properties(
    dpso_img PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(dpso_img PRIVATE -Wall -Wextra -pedantic)
endif()

target_include_directories(dpso_img PRIVATE . PUBLIC ..)

if(NOT TARGET dpso_utils)
    add_subdirectory(
        ../dpso_utils "${CMAKE_BINARY_DIR}/src/dpso_utils")
endif()

if(NOT TARGET stb_image_resize2)
    add_subdirectory(
        ../thirdparty/stb_image_resize2
        "${CMAKE_BINARY_DIR}/src/thirdparty/stb_image_resize2")
endif()

target_link_libraries(dpso_img PRIVATE dpso_utils stb_image_resize2)
