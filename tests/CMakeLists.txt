cmake_minimum_required(VERSION 2.8.12)

project(dpScreenOCR-Tests)

set(PLATFORM_SRCS)
if(WIN32)
    list(APPEND PLATFORM_SRCS test_windows_utils.cpp)
endif()

add_executable(
    tests

    flow.cpp
    main.cpp
    test_cfg.cpp
    test_geometry.cpp
    test_history.cpp
    test_hotkeys.cpp
    test_os.cpp
    test_progress_tracker.cpp
    test_str.cpp
    test_str_format.cpp
    test_tesseract_utils.cpp
    utils.cpp

    ${PLATFORM_SRCS}
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        tests PRIVATE -std=c++11 -Wall -Wextra -pedantic
    )
endif()

if(NOT TARGET dpso)
    add_subdirectory(../src/dpso ${CMAKE_BINARY_DIR}/src/dpso)
endif()

if(NOT TARGET dpso_utils)
    add_subdirectory(
        ../src/dpso_utils ${CMAKE_BINARY_DIR}/src/dpso_utils
    )
endif()

target_link_libraries(tests dpso dpso_utils)

add_executable(test_c_compilation test_c_compilation.c)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        test_c_compilation
        PRIVATE -std=c89 -Wall -Wextra -pedantic -Wstrict-prototypes
    )
endif()

if(NOT TARGET dpso_intl)
    add_subdirectory(
        ../src/dpso_intl ${CMAKE_BINARY_DIR}/src/dpso_intl
    )
endif()

if(NOT TARGET ui_common)
    add_subdirectory(
        ../src/ui/ui_common ${CMAKE_BINARY_DIR}/src/ui/ui_common
    )
endif()

if(NOT TARGET ui_taskbar)
    add_subdirectory(
        ../src/ui/ui_taskbar ${CMAKE_BINARY_DIR}/src/ui/ui_taskbar
    )
endif()

target_link_libraries(
    test_c_compilation dpso dpso_intl dpso_utils ui_common ui_taskbar
)
