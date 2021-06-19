cmake_minimum_required(VERSION 2.8.12)

project(dpScreenOCR-Tests)

set(PLATFORM_SPECIFIC_SRCS)
if(WIN32)
    list(APPEND PLATFORM_SPECIFIC_SRCS test_windows_utils.cpp)
endif()

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
    test_tesseract_utils.cpp
    utils.cpp

    ${PLATFORM_SPECIFIC_SRCS}
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        tests
        PRIVATE -std=c++11 -Wall -Wextra -pedantic
    )
endif()

target_link_libraries(tests dpso dpso_utils)

add_executable(test-c-compilation test_c_compilation.c)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        test-c-compilation
        PRIVATE -std=c89 -Wall -Wextra -pedantic -Wstrict-prototypes
    )
endif()

if(NOT TARGET common)
    add_subdirectory(../src/ui/common ${CMAKE_BINARY_DIR}/common)
endif()

target_link_libraries(test-c-compilation dpso dpso_utils common)
