
add_library(
    ui_common STATIC

    cfg_default_values.cpp
    cfg_keys.cpp
    init.cpp
    init_intl.cpp
    ocr_data_utils.cpp
    user_agent.cpp)

# Files for configure_file()
set(IN_SRCS app_info.cpp file_names.cpp)

if(UNIX AND NOT APPLE)
    target_sources(
        ui_common PRIVATE app_dirs_unix.cpp init_extra_unix.cpp)

    include(GNUInstallDirs)
    list(APPEND IN_SRCS app_dirs_unix_cfg.h)
elseif(WIN32)
    target_sources(
        ui_common
        PRIVATE
        app_dirs_windows.cpp
        init_extra_windows.cpp)
else()
    message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} is not supported")
endif()

list(APPEND IN_SRCS taskbar_config.h)
if(WIN32)
    set(UI_TASKBAR_WIN 1)
    target_sources(ui_common PRIVATE taskbar_windows.cpp)
else()
    set(UI_TASKBAR_NULL 1)
    target_sources(ui_common PRIVATE taskbar_null.cpp)
endif()

if(UNIX AND NOT APPLE)
    target_sources(ui_common PRIVATE single_instance_guard_unix.cpp)
elseif(WIN32)
    target_sources(
        ui_common PRIVATE single_instance_guard_windows.cpp)
else()
    target_sources(ui_common PRIVATE single_instance_guard_null.cpp)
endif()

foreach(IN_SRC ${IN_SRCS})
    configure_file("${IN_SRC}.in" "${IN_SRC}" @ONLY)

    get_filename_component(IN_SRC_EXT "${IN_SRC}" EXT)
    if(NOT IN_SRC_EXT STREQUAL ".h")
        target_sources(
            ui_common PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/${IN_SRC}")
    endif()
endforeach()

set_target_properties(
    ui_common PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(ui_common PRIVATE -Wall -Wextra -pedantic)
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

target_link_libraries(
    ui_common
    PUBLIC dpso
    PRIVATE dpso_intl dpso_utils)
