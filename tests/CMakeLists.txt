add_executable(
    tests

    flow.cpp
    main.cpp
    utils.cpp

    dpso_ext/test_cfg.cpp
    dpso_ext/test_history.cpp
    dpso_ext/test_history_export.cpp
    dpso_ocr/test_tesseract_utils.cpp
    dpso_sys/test_keys.cpp
    dpso_utils/stream/test_out_newline_conversion_stream.cpp
    dpso_utils/test_byte_order.cpp
    dpso_utils/test_geometry.cpp
    dpso_utils/test_line_reader.cpp
    dpso_utils/test_os.cpp
    dpso_utils/test_os_stdio.cpp
    dpso_utils/test_progress_tracker.cpp
    dpso_utils/test_sha256.cpp
    dpso_utils/test_sha256_file.cpp
    dpso_utils/test_str.cpp
    dpso_utils/test_strftime.cpp
    dpso_utils/test_version_cmp.cpp
    ui/ui_common/test_str_nformat.cpp)

if(WIN32)
    target_sources(
        tests
        PRIVATE
        dpso_sys/test_windows_action_executor.cpp
        dpso_utils/windows/test_cmdline.cpp
        dpso_utils/windows/test_utf.cpp)
endif()

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

if(NOT TARGET dpso_ext)
    add_subdirectory(
        ../src/dpso_ext "${CMAKE_BINARY_DIR}/src/dpso_ext")
endif()

if(NOT TARGET dpso_ocr)
    add_subdirectory(
        ../src/dpso_ocr "${CMAKE_BINARY_DIR}/src/dpso_ocr")
endif()

if(NOT TARGET dpso_sys)
    add_subdirectory(
        ../src/dpso_sys "${CMAKE_BINARY_DIR}/src/dpso_sys")
endif()

if(NOT TARGET dpso_utils)
    add_subdirectory(
        ../src/dpso_utils "${CMAKE_BINARY_DIR}/src/dpso_utils")
endif()

if(NOT TARGET ui_common)
    add_subdirectory(
        ../src/ui/ui_common "${CMAKE_BINARY_DIR}/src/ui/ui_common")
endif()

target_link_libraries(
    tests dpso_ext dpso_ocr dpso_sys dpso_utils ui_common)

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

if(NOT TARGET dpso_img)
    add_subdirectory(
        ../src/dpso_img "${CMAKE_BINARY_DIR}/src/dpso_img")
endif()

if(NOT TARGET dpso_intl)
    add_subdirectory(
        ../src/dpso_intl "${CMAKE_BINARY_DIR}/src/dpso_intl")
endif()

target_link_libraries(
    test_c_compilation
    dpso_ext
    dpso_img
    dpso_intl
    dpso_ocr
    dpso_sys
    dpso_utils
    ui_common)
