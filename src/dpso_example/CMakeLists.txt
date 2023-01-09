cmake_minimum_required(VERSION 3.16)

project(dpso-example)

add_executable(dpso_example main.c)

set_target_properties(
    dpso_example PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        dpso_example
        PRIVATE -Wall -Wextra -pedantic -Wstrict-prototypes)
endif()

set_target_properties(
    dpso_example
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

if(NOT TARGET dpso)
    add_subdirectory(../dpso "${CMAKE_BINARY_DIR}/src/dpso")
endif()
target_link_libraries(dpso_example dpso)
