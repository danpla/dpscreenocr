cmake_minimum_required(VERSION 3.5)

project(launcher C)

add_executable(launcher main.c)

set_target_properties(
    launcher PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO)

target_compile_options(
    launcher PRIVATE -Wall -Wextra -pedantic -Wstrict-prototypes)

target_link_libraries(launcher dl)
