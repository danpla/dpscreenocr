cmake_minimum_required(VERSION 3.22)

project(
    dp_intl
    VERSION 1.0.0
    DESCRIPTION
        "Gettext-compatible internationalization library for C++"
    HOMEPAGE_URL "https://danpla.github.io/dp_intl/")

# We don't use the standard BUILD_SHARED_LIBS CMake option to avoid
# potential conflicts when the library is added to another project via
# add_subdirectory() or FetchContent.
option(
    DP_INTL_BUILD_SHARED_LIB
    "Build a shared library instead of a static one"
    NO)

if(DP_INTL_BUILD_SHARED_LIB)
    add_library(dp_intl SHARED)
else()
    add_library(dp_intl STATIC)
endif()

target_sources(
    dp_intl PRIVATE

    src/file_stream.cpp
    src/gnu_header.cpp
    src/lang_match.cpp
    src/lang_tag_utils.cpp
    src/message_catalog.cpp
    src/plural/eval.cpp
    src/plural/tokenizer.cpp
    src/str_utils.cpp
    src/utf.cpp)

add_library(dp_intl::dp_intl ALIAS dp_intl)

set_target_properties(
    dp_intl PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    DEFINE_SYMBOL DP_INTL_EXPORTS)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(dp_intl PRIVATE -Wall -Wextra -pedantic)

    if(DP_INTL_BUILD_SHARED_LIB)
        set_target_properties(
            dp_intl PROPERTIES CXX_VISIBILITY_PRESET "hidden")
    endif()
endif()

target_include_directories(
    dp_intl
    PRIVATE src
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

if(DP_INTL_BUILD_SHARED_LIB)
    target_compile_definitions(dp_intl PUBLIC DP_INTL_SHARED_LIB)
endif()

# This is the end of the mandatory part of the CMake file. Projects
# that add our library via add_subdirectory()/FetchContent can remove
# directories that are only relevant for a separate build/install, so
# all features that depend on such directories are enabled
# conditionally.

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
        include(uninstall)
    endif()

    option(DP_INTL_BUILD_DOCS "Build documentation with Doxygen" NO)
    if(DP_INTL_BUILD_DOCS)
        include(docs)
    endif()
endif()
