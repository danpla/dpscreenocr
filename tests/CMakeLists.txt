cmake_minimum_required(VERSION 2.8.12)

project(dpScreenOCR-Tests)

add_executable(
    tests

    flow.cpp
    main.cpp
    test_cfg.cpp
    test_geometry.cpp
    test_history.cpp
    test_hotkeys.cpp
    test_str.cpp
    test_str_format.cpp
    utils.cpp
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        tests
        PRIVATE -std=c++11 -Wall -Wextra -pedantic
    )
endif()

add_dependencies(tests dpso)
target_link_libraries(tests dpso)

add_dependencies(tests dpso_utils)
target_link_libraries(tests dpso_utils)


add_executable(test-c-compilation test_c_compilation.c)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        test-c-compilation
        PRIVATE -std=c89 -Wall -Wextra -pedantic
    )
endif()

add_dependencies(test-c-compilation dpso)
target_link_libraries(test-c-compilation dpso)

add_dependencies(test-c-compilation dpso_utils)
target_link_libraries(test-c-compilation dpso_utils)

if (NOT TARGET common)
    add_subdirectory(../src/ui/common ${CMAKE_BINARY_DIR}/common)
endif()
add_dependencies(test-c-compilation common)
target_link_libraries(test-c-compilation common)
