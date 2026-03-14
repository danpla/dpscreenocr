cmake_minimum_required(VERSION 3.22)

project(dpso_intl)

add_library(
    dpso_intl STATIC

    bindtextdomain_utf8.cpp
    helpers.cpp)

set_target_properties(
    dpso_intl PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(dpso_intl PRIVATE -Wall -Wextra -pedantic)
endif()

target_include_directories(dpso_intl PRIVATE . PUBLIC ..)

if(WIN32)
    # We need at least 0.21 for wbindtextdomain()
    find_package(Intl 0.21.0 REQUIRED)

    if(NOT TARGET dpso_utils)
        add_subdirectory(
            ../dpso_utils "${CMAKE_BINARY_DIR}/src/dpso_utils")
    endif()
    target_link_libraries(dpso_intl PRIVATE dpso_utils)
else()
    find_package(Intl REQUIRED)
endif()

target_include_directories(dpso_intl PUBLIC ${Intl_INCLUDE_DIRS})
target_link_libraries(dpso_intl PUBLIC ${Intl_LIBRARIES})
