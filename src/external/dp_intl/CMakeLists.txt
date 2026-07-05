cmake_minimum_required(VERSION 3.22)

project(
    dp_intl
    VERSION 0.9.0
    DESCRIPTION
        "Gettext-compatible internationalization library for C++")

add_library(
    dp_intl

    src/file_data_stream.cpp
    src/gnu_header.cpp
    src/lang_match.cpp
    src/lang_tag_utils.cpp
    src/message_catalog.cpp
    src/plural/eval.cpp
    src/plural/tokenizer.cpp
    src/str_utils.cpp)

add_library(dp_intl::dp_intl ALIAS dp_intl)

set_target_properties(
    dp_intl PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        dp_intl PRIVATE -Wall -Wextra -pedantic)
endif()

target_include_directories(
    dp_intl
    PRIVATE src
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# This is the end of the mandatory part of the CMake file. Projects
# that add our library via add_subdirectory() can remove directories
# that are only relevant for separate build/install, so all features
# that depend on such directories are enabled conditionally.

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    option(DP_INTL_BUILD_TESTS "Build tests" NO)
    if(DP_INTL_BUILD_TESTS)
        add_subdirectory(tests)
    endif()
endif()

if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

    option(
        DP_INTL_INSTALL
        "Create rules for the install target"
        "${PROJECT_IS_TOP_LEVEL}")
    if(DP_INTL_INSTALL)
        include(install)
    endif()

    option(DP_INTL_BUILD_DOCS "Build documentation with Doxygen" NO)
    if(DP_INTL_BUILD_DOCS)
        include(build_docs)
    endif()
endif()
