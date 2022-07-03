cmake_minimum_required(VERSION 2.8.12)

project(dpso_intl)

option(DPSO_ENABLE_NLS "Enable native language support" YES)

add_library(
    dpso_intl STATIC dpso_intl.cpp dpso_bindtextdomain_utf8.cpp
)

target_compile_definitions(
    dpso_intl PUBLIC ENABLE_NLS=$<BOOL:${DPSO_ENABLE_NLS}>
)

target_include_directories(dpso_intl PRIVATE . PUBLIC ..)

if(DPSO_ENABLE_NLS)
    # FindIntl was added in CMake 3.2.
    if(CMAKE_VERSION VERSION_LESS 3.2)
        find_path(Intl_INCLUDE_DIRS libintl.h)
        mark_as_advanced(Intl_INCLUDE_DIRS)
        if(NOT Intl_INCLUDE_DIRS)
            message(SEND_ERROR "Can't find libintl.h")
        endif()

        find_library(Intl_LIBRARY intl)
        mark_as_advanced(Intl_LIBRARY)
        if(Intl_LIBRARY)
            # intl may be a part of libc rather than a separate
            # library.
            set(Intl_LIBRARIES "${Intl_LIBRARY}")
        endif()
    else()
        if(WIN32)
            # We need at least 0.21 for wbindtextdomain()
            find_package(Intl 0.21.0 REQUIRED)
        else()
            find_package(Intl REQUIRED)
        endif()
    endif()

    target_include_directories(dpso_intl PUBLIC ${Intl_INCLUDE_DIRS})
    target_link_libraries(dpso_intl ${Intl_LIBRARIES})
endif()
