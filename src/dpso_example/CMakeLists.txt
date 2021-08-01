cmake_minimum_required(VERSION 2.8.12)

project(dpso-example)

add_executable(dpso-example main.c)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        dpso-example
        PRIVATE -std=c89 -Wall -Wextra -pedantic -Wstrict-prototypes
    )
endif()

set_target_properties(
    dpso-example
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

target_include_directories(dpso-example PRIVATE .)

if(NOT TARGET dpso)
    add_subdirectory(../dpso ${CMAKE_BINARY_DIR}/src/dpso)
endif()
target_link_libraries(dpso-example dpso)
