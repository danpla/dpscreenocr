
set(DPSO_QT_VERSION "5" CACHE STRING "Qt version to use: 5 or 6")
set_property(CACHE DPSO_QT_VERSION PROPERTY STRINGS 5 6)

add_executable(
    dpscreenocr_qt

    about.cpp
    action_chooser.cpp
    history.cpp
    hotkey_editor.cpp
    lang_browser.cpp
    main.cpp
    main_window.cpp
    status_indicator.cpp
    utils.cpp)

if(WIN32)
    add_subdirectory(
        ../windows_rc "${CMAKE_BINARY_DIR}/src/ui/windows_rc")
    target_sources(
        dpscreenocr_qt PRIVATE $<TARGET_OBJECTS:windows_rc>)
endif()

set_target_properties(
    dpscreenocr_qt PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    AUTOMOC ON
    OUTPUT_NAME "dpscreenocr"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        dpscreenocr_qt PRIVATE -Wall -Wextra -pedantic)
endif()

target_include_directories(dpscreenocr_qt PRIVATE .)

if(CMAKE_BUILD_TYPE MATCHES "Release|RelWithDebInfo|MinSizeRel")
    target_compile_definitions(dpscreenocr_qt PRIVATE QT_NO_DEBUG=1)

    if(WIN32)
        # Hide console window.
        set_target_properties(
            dpscreenocr_qt PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
endif()

if(NOT TARGET dpso)
    add_subdirectory(../../dpso "${CMAKE_BINARY_DIR}/src/dpso")
endif()

if(NOT TARGET dpso_ext)
    add_subdirectory(
        ../../dpso_ext "${CMAKE_BINARY_DIR}/src/dpso_ext")
endif()

if(NOT TARGET dpso_intl)
    add_subdirectory(
        ../../dpso_intl "${CMAKE_BINARY_DIR}/src/dpso_intl")
endif()

if(NOT TARGET dpso_utils)
    add_subdirectory(
        ../../dpso_utils "${CMAKE_BINARY_DIR}/src/dpso_utils")
endif()

if(NOT TARGET ui_common)
    add_subdirectory(
        ../ui_common "${CMAKE_BINARY_DIR}/src/ui/ui_common")
endif()

target_link_libraries(
    dpscreenocr_qt dpso dpso_ext dpso_intl dpso_utils ui_common)

if(DPSO_QT_VERSION EQUAL 5 OR DPSO_QT_VERSION EQUAL 6)
    find_package(
        Qt${DPSO_QT_VERSION} REQUIRED COMPONENTS Core Gui Widgets)

    # Qt 5 < 5.3.2 is affected by a window title bug:
    # https://bugreports.qt.io/browse/QTBUG-33775
    if (DPSO_QT_VERSION EQUAL 5
            AND Qt5Widgets_VERSION VERSION_LESS "5.3.2")
        message(FATAL_ERROR "Qt 5 >= 5.3.2 is required")
    endif()

    target_link_libraries(
        dpscreenocr_qt
        Qt${DPSO_QT_VERSION}::Core
        Qt${DPSO_QT_VERSION}::Gui
        Qt${DPSO_QT_VERSION}::Widgets)
else()
    message(FATAL_ERROR "Invalid Qt version \"${DPSO_QT_VERSION}\"")
endif()
