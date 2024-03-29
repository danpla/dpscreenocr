cmake_minimum_required(VERSION 3.5)

project(dpso_intl)

option(DPSO_ENABLE_NLS "Enable native language support" YES)

add_library(
    dpso_intl STATIC

    bindtextdomain_utf8.cpp
    helpers.cpp
    libintl_noop.cpp)

target_compile_definitions(
    dpso_intl PUBLIC ENABLE_NLS=$<BOOL:${DPSO_ENABLE_NLS}>)

set_target_properties(
    dpso_intl PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(dpso_intl PRIVATE -Wall -Wextra -pedantic)
endif()

target_include_directories(dpso_intl PRIVATE . PUBLIC ..)

if(DPSO_ENABLE_NLS)
    if(WIN32)
        # We need at least 0.21 for wbindtextdomain()
        find_package(Intl 0.21.0 REQUIRED)
    else()
        find_package(Intl REQUIRED)
    endif()

    target_include_directories(dpso_intl PUBLIC ${Intl_INCLUDE_DIRS})
    target_link_libraries(dpso_intl PUBLIC ${Intl_LIBRARIES})
endif()
