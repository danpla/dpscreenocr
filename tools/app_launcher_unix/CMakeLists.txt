cmake_minimum_required(VERSION 3.5)

project(launcher C)

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

target_link_libraries(launcher dl)
