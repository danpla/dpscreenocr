# 3.22 is the version available in Ubuntu 22.04.
cmake_minimum_required(VERSION 3.22)

project(dpScreenOCR)

include(CMakeDependentOption)

set(DPSO_UI "qt" CACHE STRING "UI to use: qt or none")
set_property(CACHE DPSO_UI PROPERTY STRINGS qt none)
option(DPSO_GEN_HTML_MANUAL "Generate HTML manual using Pandoc" YES)
cmake_dependent_option(
    DPSO_ENABLE_MSIX
    "Enable support for building MSIX package"
    NO
    "WIN32"
    NO)
option(DPSO_BUILD_EXAMPLE "Build example" NO)
option(DPSO_BUILD_TESTS "Build tests" NO)

set(APP_NAME "dpScreenOCR")
set(APP_FILE_NAME "dpscreenocr")

set(APP_VERSION_MAJOR "1")
set(APP_VERSION_MINOR "5")
set(APP_VERSION_PATCH "0")
set(APP_VERSION
    "${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH}")

set(APP_DESCRIPTION "A program to recognize text on the screen")

set(APP_AUTHOR "Danyl Plakhotych")
set(APP_COPYRIGHT_YEAR "2019-2026")
set(APP_URL "https://danpla.github.io/dpscreenocr/")

set(APP_MSIX_NAME "DanylPlakhotych.8735E3B22608")
set(APP_MSIX_PUBLISHER "CN=4F28353A-E1C7-4512-B80D-5967CA05FF40")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(fixup_gnu_install_dirs)

if(DPSO_BUILD_EXAMPLE)
    add_subdirectory(src/dpso_example)
endif()

if(DPSO_UI STREQUAL "qt")
    add_subdirectory(src/ui/qt)
elseif(NOT DPSO_UI STREQUAL "none")
    message(FATAL_ERROR "Unknown UI \"${DPSO_UI}\"")
endif()

if(DPSO_BUILD_TESTS)
    add_subdirectory(tests)
endif()

include(install)
include(uninstall)
include(dist)
