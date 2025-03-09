cmake_minimum_required(VERSION 3.22)

project(dpso_net)

add_library(
    dpso_net

    download_file.cpp
    get_data.cpp)

include(CMakeDependentOption)

string(
    CONCAT DPSO_DYNAMIC_CURL_DESCRIPTION
    "Load libcurl dynamically. You should enable this option if the "
    "program is going to be shipped in a self-contained package.")
cmake_dependent_option(
    DPSO_DYNAMIC_CURL
    "${DPSO_DYNAMIC_CURL_DESCRIPTION}"
    NO
    "UNIX; NOT APPLE"
    NO)

if(WIN32)
    target_sources(dpso_net PRIVATE request_windows.cpp)
    target_link_libraries(dpso_net PRIVATE wininet)
else()
    target_sources(
        dpso_net PRIVATE request_curl.cpp request_curl_lib.cpp)

    # curl_multi_wait() was added in 7.28.
    find_package(CURL 7.28 REQUIRED)
    target_include_directories(dpso_net PRIVATE ${CURL_INCLUDE_DIRS})

    target_compile_definitions(
        dpso_net
        PRIVATE
        DPSO_DYNAMIC_CURL=$<BOOL:${DPSO_DYNAMIC_CURL}>)

    if(DPSO_DYNAMIC_CURL)
        target_link_libraries(dpso_net PRIVATE dl)
    else()
        target_link_libraries(dpso_net PRIVATE ${CURL_LIBRARIES})
    endif()
endif()

set_target_properties(
    dpso_net PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(dpso_net PRIVATE -Wall -Wextra -pedantic)
endif()

target_include_directories(dpso_net PRIVATE . PUBLIC ..)

if(NOT TARGET dpso_utils)
    add_subdirectory(
        ../dpso_utils "${CMAKE_BINARY_DIR}/src/dpso_utils")
endif()

target_link_libraries(dpso_net PRIVATE dpso_utils)
