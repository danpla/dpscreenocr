cmake_minimum_required(VERSION 2.8.12)

project(ui_common)

if(UNIX AND NOT APPLE)
    include(GNUInstallDirs)
    set(APP_LOCALE_DIR "${CMAKE_INSTALL_FULL_LOCALEDIR}")

    # Construct doc dir manually since DOCDIR uses PROJECT_NAME.
    set(
        APP_DOC_DIR
        "${CMAKE_INSTALL_FULL_DATADIR}/doc/${APP_FILE_NAME}"
    )
else()
    set(APP_LOCALE_DIR "")
    set(APP_DOC_DIR "")
endif()

set(IN_SRCS app_info.cpp file_names.cpp paths.cpp)
set(CONFIGURED_SRCS)
foreach(IN_SRC ${IN_SRCS})
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/${IN_SRC}.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${IN_SRC}"
        @ONLY
    )

    list(
        APPEND
        CONFIGURED_SRCS "${CMAKE_CURRENT_BINARY_DIR}/${IN_SRC}"
    )
endforeach()

set(PLATFORM_SRCS)
if(WIN32)
    list(APPEND PLATFORM_SRCS startup_setup_windows.cpp)
else()
    list(APPEND PLATFORM_SRCS startup_setup_null.cpp)
endif()

add_library(
    ui_common STATIC

    cfg_default_values.cpp
    cfg_keys.cpp
    ocr_data_dir_name.cpp
    ${CONFIGURED_SRCS}
    ${PLATFORM_SRCS}
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        ui_common PRIVATE -std=c++11 -Wall -Wextra -pedantic
    )
endif()

target_include_directories(ui_common PRIVATE . PUBLIC ..)

if(NOT TARGET dpso)
    add_subdirectory(../../dpso ${CMAKE_BINARY_DIR}/src/dpso)
endif()

if(NOT TARGET dpso_utils)
    add_subdirectory(
        ../../dpso_utils
        ${CMAKE_BINARY_DIR}/src/dpso_utils
    )
endif()

target_link_libraries(ui_common dpso dpso_utils)
