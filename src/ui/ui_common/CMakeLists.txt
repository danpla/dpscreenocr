
set(SRCS
    cfg_default_values.cpp
    cfg_keys.cpp
    init_intl.cpp
    ocr_data_dir_name.cpp)

# Files for configure_file()
set(IN_SRCS app_info.cpp file_names.cpp icon_names.cpp)

if(UNIX AND NOT APPLE)
    list(APPEND SRCS startup_setup_null.cpp dirs_unix.cpp)

    include(GNUInstallDirs)
    list(APPEND IN_SRCS dirs_unix_cfg.h)
elseif(WIN32)
    list(
        APPEND SRCS
        dirs_windows.cpp
        startup_setup_windows.cpp)
else()
    message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} is not supported")
endif()

list(APPEND IN_SRCS taskbar_config.h)
if(WIN32)
    set(UI_TASKBAR_WIN 1)
    list(APPEND SRCS taskbar_windows.cpp)
else()
    set(UI_TASKBAR_NULL 1)
    list(APPEND SRCS taskbar_null.cpp)
endif()

foreach(IN_SRC ${IN_SRCS})
    configure_file("${IN_SRC}.in" "${IN_SRC}" @ONLY)

    get_filename_component(IN_SRC_EXT "${IN_SRC}" EXT)
    if(NOT IN_SRC_EXT STREQUAL ".h")
        list(APPEND SRCS "${CMAKE_CURRENT_BINARY_DIR}/${IN_SRC}")
    endif()
endforeach()

add_library(ui_common STATIC ${SRCS})

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        ui_common PRIVATE -std=c++11 -Wall -Wextra -pedantic)
endif()

target_include_directories(
    ui_common
    PRIVATE .
    PUBLIC .. "${CMAKE_CURRENT_BINARY_DIR}")

if(NOT TARGET dpso)
    add_subdirectory(../../dpso "${CMAKE_BINARY_DIR}/src/dpso")
endif()

if(NOT TARGET dpso_intl)
    add_subdirectory(
        ../../dpso_intl "${CMAKE_BINARY_DIR}/src/dpso_intl")
endif()

if(NOT TARGET dpso_utils)
    add_subdirectory(
        ../../dpso_utils "${CMAKE_BINARY_DIR}/src/dpso_utils")
endif()

target_link_libraries(ui_common dpso dpso_intl dpso_utils)
