# We want the default configuration on Unix-like systems to be
# suitable for building the app for system package managers. In this
# case, the package manager will check for updates, and our internal
# update checking is unnecessary.
if(NOT DEFINED DPSO_ENABLE_UPDATE_CHECKER AND UNIX AND NOT APPLE)
    set(DPSO_ENABLE_UPDATE_CHECKER NO CACHE BOOL "")
endif()

option(
    DPSO_ENABLE_UPDATE_CHECKER
    "Enable functionality to check for new app versions"
    YES)

add_library(
    ui_common STATIC

    autostart_default.cpp
    cfg_default_values.cpp
    cfg_keys.cpp
    cmdline_cmd_autostart.cpp
    cmdline_opts.cpp
    init.cpp
    init_intl.cpp
    init_startup_args.cpp
    ocr_data_utils.cpp
    sound.cpp
    str_nformat.cpp
    toplevel_argv0.cpp
    update_checker_default.cpp
    user_agent.cpp)

# Files for configure_file()
set(IN_SRCS app_info.cpp file_names.cpp)

if(UNIX AND NOT APPLE)
    target_sources(
        ui_common
        PRIVATE
        app_dirs_unix.cpp
        autostart_unix.cpp
        exe_path_unix.cpp
        init_extra_unix.cpp)

    include(GNUInstallDirs)
    list(APPEND IN_SRCS app_dirs_unix_cfg.h)
elseif(WIN32)
    target_sources(
        ui_common
        PRIVATE
        app_dirs_windows.cpp
        autostart_windows.cpp
        exe_path_windows.cpp
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
endif()

if(DPSO_ENABLE_UPDATE_CHECKER)
    target_sources(ui_common PRIVATE update_checker.cpp)

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_sources(
            ui_common PRIVATE update_checker_platform_linux.cpp)
    elseif(WIN32)
        target_sources(
            ui_common PRIVATE update_checker_platform_windows.cpp)
    else()
        target_sources(
            ui_common PRIVATE update_checker_platform_generic.cpp)
    endif()

    if(NOT TARGET dpso_json)
        add_subdirectory(
            ../dpso_json "${CMAKE_BINARY_DIR}/src/dpso_json")
    endif()

    if(NOT TARGET dpso_net)
        add_subdirectory(
            ../dpso_net "${CMAKE_BINARY_DIR}/src/dpso_net")
    endif()

    find_package(Threads REQUIRED)

    target_link_libraries(
        ui_common
        PRIVATE dpso_json dpso_net ${CMAKE_THREAD_LIBS_INIT})
else()
    target_sources(dpso PRIVATE update_checker_null.cpp)
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

if(NOT TARGET dpso_intl)
    add_subdirectory(
        ../../dpso_intl "${CMAKE_BINARY_DIR}/src/dpso_intl")
endif()

if(NOT TARGET dpso_ocr)
    add_subdirectory(
        ../../dpso_ocr "${CMAKE_BINARY_DIR}/src/dpso_ocr")
endif()

if(NOT TARGET dpso_sound)
    add_subdirectory(
        ../../dpso_sound "${CMAKE_BINARY_DIR}/src/dpso_sound")
endif()

if(NOT TARGET dpso_sys)
    add_subdirectory(
        ../../dpso_sys "${CMAKE_BINARY_DIR}/src/dpso_sys")
endif()

if(NOT TARGET dpso_utils)
    add_subdirectory(
        ../../dpso_utils "${CMAKE_BINARY_DIR}/src/dpso_utils")
endif()

target_link_libraries(
    ui_common
    PUBLIC dpso_ocr dpso_sys dpso_utils
    PRIVATE dpso_intl dpso_sound)
