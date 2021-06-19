cmake_minimum_required(VERSION 2.8.12)

project(dpScreenOCR-QT)

set(DPSO_QT_VERSION "5" CACHE STRING "Qt version to use: 4 or 5")
set_property(CACHE DPSO_QT_VERSION PROPERTY STRINGS 4 5)
option(
    DPSO_QT_LOCAL_DATA
    "Allow to load data (e.g. locales) from the Qt executable's dir"
    YES
)

# See https://bugreports.qt.io/browse/QTBUG-33775. This bug doesn't
# affect Qt 4.
option(
    DPSO_QT_X11_SET_WINDOW_TITLE_WORKAROUND
    "Use workaround for setWindowTitle() bug in Qt 5 < 5.3.2 with X11"
    YES
)

# CMP0071: Let AUTOMOC and AUTOUIC process GENERATED files.
# We don't rely on this behavior; we just need to suppress the warning,
# as it's triggered for the generated .rcc files.
if(POLICY CMP0071)
    cmake_policy(SET CMP0071 NEW)
endif()

set(
    SRCS

    about.cpp
    action_chooser.cpp
    history.cpp
    hotkey_editor.cpp
    lang_browser.cpp
    main.cpp
    main_window.cpp
    status_indicator.cpp
    utils.cpp
)

if(WIN32)
    add_subdirectory(../windows_rc ${CMAKE_BINARY_DIR}/windows_rc)
    list(APPEND SRCS $<TARGET_OBJECTS:windows_rc>)
endif()

# qtX_ macros come from QtCore
if(DPSO_QT_VERSION EQUAL 4)
    find_package(Qt4 REQUIRED COMPONENTS QtCore QtGui)
elseif(DPSO_QT_VERSION EQUAL 5)
    find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)
else()
    message(FATAL_ERROR "Invalid Qt version \"${DPSO_QT_VERSION}\"")
endif()

# Add libraries early to avoid AUTOMOC
if(NOT TARGET common)
    add_subdirectory(../common ${CMAKE_BINARY_DIR}/common)
endif()

if(NOT TARGET dpso_intl)
    add_subdirectory(../../dpso_intl ${CMAKE_BINARY_DIR}/dpso_intl)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

add_executable(dpscreenocr-qt ${SRCS})

target_compile_definitions(
    dpscreenocr-qt
    PRIVATE
    DPSO_QT_LOCAL_DATA=$<BOOL:${DPSO_QT_LOCAL_DATA}>
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        dpscreenocr-qt
        PRIVATE -std=c++11 -Wall -Wextra -pedantic
    )
endif()

set_target_properties(
    dpscreenocr-qt
    PROPERTIES
    OUTPUT_NAME "dpscreenocr"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)

if(WIN32 AND CMAKE_BUILD_TYPE MATCHES "Release|RelWithDebInfo|MinSizeRel")
    # Hide console window.
    set_target_properties(
        dpscreenocr-qt
        PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

target_link_libraries(dpscreenocr-qt common dpso dpso_utils dpso_intl)

if(DPSO_QT_VERSION EQUAL 4)
    target_link_libraries(dpscreenocr-qt Qt4::QtCore Qt4::QtGui)
else()
    target_link_libraries(
        dpscreenocr-qt Qt5::Core Qt5::Gui Qt5::Widgets
    )
endif()

if(NOT DPSO_QT_VERSION EQUAL 4
        AND DPSO_QT_X11_SET_WINDOW_TITLE_WORKAROUND
        AND Qt5Widgets_VERSION VERSION_LESS "5.3.2")
    target_compile_definitions(
        dpscreenocr-qt
        PRIVATE
        DPSO_QT_X11_SET_WINDOW_TITLE_WORKAROUND=1
    )

    find_package(Qt5X11Extras REQUIRED)
    target_link_libraries(dpscreenocr-qt Qt5::X11Extras)

    find_package(X11 REQUIRED)
    target_include_directories(
        dpscreenocr-qt
        PRIVATE
        ${X11_INCLUDE_DIR}
    )
    target_link_libraries(dpscreenocr-qt ${X11_LIBRARIES})
else()
    target_compile_definitions(
        dpscreenocr-qt
        PRIVATE
        DPSO_QT_X11_SET_WINDOW_TITLE_WORKAROUND=0
    )
endif()
