cmake_minimum_required(VERSION 3.16)

project(dpso_json)

add_library(dpso_json json.cpp)

set_target_properties(
    dpso_json PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(dpso_json PRIVATE -Wall -Wextra -pedantic)
endif()

target_include_directories(dpso_json PRIVATE . PUBLIC ..)

if(NOT TARGET dpso_utils)
    add_subdirectory(
        ../dpso_utils "${CMAKE_BINARY_DIR}/src/dpso_utils")
endif()

target_link_libraries(dpso_json PRIVATE dpso_utils)

find_package(PkgConfig REQUIRED)

# json_string_length() was added in 2.7.
pkg_search_module(JANSSON REQUIRED jansson>=2.7)

target_include_directories(dpso_json PRIVATE ${JANSSON_INCLUDE_DIRS})
target_link_libraries(dpso_json PRIVATE dpso_net ${JANSSON_LIBRARIES})
