cmake_minimum_required(VERSION 3.22)

project(dpso_json)

add_library(dpso_json json.cpp)

set_target_properties(
    dpso_json PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(dpso_json PRIVATE -Wall -Wextra -pedantic)
endif()

find_package(PkgConfig REQUIRED)

# Functions with fixed-length keys like json_object_getn() were added
# in 2.14.
pkg_search_module(JANSSON REQUIRED jansson>=2.14)

if(NOT TARGET dpso_utils)
    add_subdirectory(
        ../dpso_utils "${CMAKE_BINARY_DIR}/src/dpso_utils")
endif()

target_include_directories(
    dpso_json PUBLIC .. PRIVATE . ${JANSSON_INCLUDE_DIRS})

target_link_libraries(
    dpso_json PRIVATE dpso_utils ${JANSSON_LIBRARIES})
