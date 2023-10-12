cmake_minimum_required(VERSION 3.5)

project(launcher C)

set(LAUNCHER_EXE_PATH
    ""
    CACHE
    STRING
    "Relative path to an executable to be launched")
set(LAUNCHER_LIB_DIR_PATH
    ""
    CACHE
    STRING
    "Relative path to a directory with libraries")

function(validate_launcher_path VAR_NAME)
    if(${VAR_NAME} STREQUAL "")
        message(SEND_ERROR "${VAR_NAME} is not set")
    elseif(IS_ABSOLUTE "${${VAR_NAME}}")
        message(SEND_ERROR "${VAR_NAME} must not be absolute")
    endif()
endfunction()

validate_launcher_path(LAUNCHER_EXE_PATH)
validate_launcher_path(LAUNCHER_LIB_DIR_PATH)

add_executable(launcher main.c)

set_target_properties(
    launcher PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        launcher
        PRIVATE -Wall -Wextra -pedantic -Wstrict-prototypes)
endif()

target_compile_definitions(
    launcher
    PRIVATE
    LAUNCHER_EXE_PATH="${LAUNCHER_EXE_PATH}"
    LAUNCHER_LIB_DIR_PATH="${LAUNCHER_LIB_DIR_PATH}")

target_link_libraries(launcher dl)
