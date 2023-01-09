
set(PLATFORM_SRCS)
if(WIN32)
    list(APPEND PLATFORM_SRCS dpso_utils/test_windows_utils.cpp)
endif()

add_executable(
    tests

    dpso/test_geometry.cpp
    dpso/test_hotkeys.cpp
    dpso/test_progress_tracker.cpp
    dpso/test_str.cpp
    dpso/test_strftime.cpp
    dpso/test_tesseract_utils.cpp
    dpso_utils/test_cfg.cpp
    dpso_utils/test_history.cpp
    dpso_utils/test_history_export.cpp
    dpso_utils/test_os.cpp
    dpso_utils/test_str_format.cpp
    flow.cpp
    main.cpp
    utils.cpp

    ${PLATFORM_SRCS})

set_target_properties(
    tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(tests PRIVATE -Wall -Wextra -pedantic)
endif()

target_include_directories(tests PRIVATE .)

if(NOT TARGET dpso)
    add_subdirectory(../src/dpso "${CMAKE_BINARY_DIR}/src/dpso")
endif()

if(NOT TARGET dpso_utils)
    add_subdirectory(
        ../src/dpso_utils "${CMAKE_BINARY_DIR}/src/dpso_utils")
endif()

target_link_libraries(tests dpso dpso_utils)

add_executable(test_c_compilation test_c_compilation.c)

set_target_properties(
    test_c_compilation PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        test_c_compilation
        PRIVATE -Wall -Wextra -pedantic -Wstrict-prototypes)
endif()

if(NOT TARGET dpso_intl)
    add_subdirectory(
        ../src/dpso_intl "${CMAKE_BINARY_DIR}/src/dpso_intl")
endif()

if(NOT TARGET ui_common)
    add_subdirectory(
        ../src/ui/ui_common "${CMAKE_BINARY_DIR}/src/ui/ui_common")
endif()

target_link_libraries(
    test_c_compilation dpso dpso_intl dpso_utils ui_common)
