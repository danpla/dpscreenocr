cmake_minimum_required(VERSION 2.8.12)

project(dpScreenOCR-FLTK)

add_executable(
    dpso-fltk

    main.cpp
    hotkey_button.cpp
    lang_browser.cpp
)

target_compile_options(
    dpso-fltk
    PRIVATE -std=c++11 -Wall -Wextra -pedantic
)

set_target_properties(
    dpso-fltk
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

add_dependencies(dpso-fltk dpso)
target_link_libraries(dpso-fltk dpso)


find_package(FLTK REQUIRED NO_MODULE)
include(${FLTK_USE_FILE})
#target_link_libraries(dpso-fltk fltk Xext dl)
target_link_libraries(dpso-fltk fltk_SHARED)
