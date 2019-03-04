cmake_minimum_required(VERSION 2.8.12)

project(dpso-example)

add_executable(dpso-example main.c)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        dpso-example
        PRIVATE -std=c89 -Wall -Wextra -pedantic
    )
endif()

set_target_properties(
    dpso-example
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

target_include_directories(dpso-example PRIVATE .)

add_dependencies(dpso-example dpso)
target_link_libraries(dpso-example dpso)


if (UNIX OR WIN32)
    add_library(dpso-example-delay STATIC delay.c)
else()
    # C++11 fallback that use std::this_thread::sleep_for()
    add_library(dpso-example-delay STATIC delay_std.cpp)

    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(
            dpso-example-delay
            PRIVATE -std=c++11 -Wall -Wextra -pedantic
        )
    endif()

    set(CMAKE_THREAD_PREFER_PTHREAD True)
    find_package(Threads REQUIRED)
    if(CMAKE_USE_PTHREADS_INIT)
        target_compile_options(
            dpso-example-delay PRIVATE -pthread
        )
    endif()

    target_link_libraries(
        dpso-example-delay ${CMAKE_THREAD_LIBS_INIT}
    )
endif()

target_include_directories(dpso-example-delay PRIVATE .)
target_include_directories(dpso-example-delay PUBLIC .)


target_link_libraries(dpso-example dpso-example-delay)
