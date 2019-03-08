cmake_minimum_required(VERSION 2.8.12)

project(dpScreenOCR-QT)

option(DPSO_QT4 "Use Qt 4 instead of Qt 5" NO)
option(
    DPSO_QT_LOCAL_DATA
    "Allow to load data (e.g. locales) from the Qt executable's dir"
    YES
)
option(DPSO_QT_RCC_ICONS "Embed icons in Qt executable" YES)

# See https://bugreports.qt.io/browse/QTBUG-33775. This bug doesn't
# affect Qt 4.
option(
    DPSO_QT_X11_SET_WINDOW_TITLE_WORKAROUND
    "Use workaround for setWindowTitle() bug in Qt 5 < 5.3.2 with X11"
    YES
)

# Add the common lib early to avoid AUTOMOC
if (NOT TARGET common)
    add_subdirectory(../common ${CMAKE_BINARY_DIR}/common)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

# CMP0071: Let AUTOMOC and AUTOUIC process GENERATED files.
# We don't rely on this behavior; we just need to suppress the warning,
# as it's triggered for the generated .rcc files.
if(POLICY CMP0071)
    cmake_policy(SET CMP0071 NEW)
endif()

set(
    SRCS

    about.cpp
    history.cpp
    hotkey_editor.cpp
    lang_browser.cpp
    main.cpp
    main_window.cpp
    status_indicator.cpp
    utils.cpp
)

# qtX_ macros come from QtCore
if (DPSO_QT4)
    find_package(Qt4 REQUIRED COMPONENTS QtCore QtGui)
else()
    find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)
endif()

if (DPSO_QT_RCC_ICONS)
    # CMake 2.8.12 doesn't have CMAKE_AUTORCC
    set(QRC_ICONS "${CMAKE_SOURCE_DIR}/data/icons/icons.qrc")
    if (DPSO_QT4)
        qt4_add_resources(RCC_ICONS "${QRC_ICONS}")
    else()
        qt5_add_resources(RCC_ICONS "${QRC_ICONS}")
    endif()
    # target_sources() is added in CMake 3.1
    set(SRCS ${SRCS} ${RCC_ICONS})
endif()

add_executable(dpscreenocr-qt ${SRCS})

target_compile_definitions(
    dpscreenocr-qt
    PRIVATE
    DPSO_QT_LOCAL_DATA=$<BOOL:${DPSO_QT_LOCAL_DATA}>
    DPSO_QT_RCC_ICONS=$<BOOL:${DPSO_QT_RCC_ICONS}>
)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
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

target_link_libraries(dpscreenocr-qt common)

add_dependencies(dpscreenocr-qt dpso)
target_link_libraries(dpscreenocr-qt dpso)

add_dependencies(dpscreenocr-qt dpso_utils)
target_link_libraries(dpscreenocr-qt dpso_utils)

if (DPSO_QT4)
    target_link_libraries(dpscreenocr-qt Qt4::QtCore Qt4::QtGui)
else()
    target_link_libraries(
        dpscreenocr-qt Qt5::Core Qt5::Gui Qt5::Widgets
    )
endif()

if (NOT DPSO_QT4
        AND DPSO_QT_X11_SET_WINDOW_TITLE_WORKAROUND
        AND Qt5Widgets_VERSION VERSION_LESS "5.3.2")
    find_package(Qt5X11Extras REQUIRED)
    target_link_libraries(dpscreenocr-qt Qt5::X11Extras)

    find_package(X11 REQUIRED)
    target_include_directories(
        dpscreenocr-qt
        PRIVATE
        ${X11_INCLUDE_DIR}
    )
    target_link_libraries(dpscreenocr-qt ${X11_LIBRARIES})

    target_compile_definitions(
        dpscreenocr-qt
        PRIVATE
        DPSO_QT_X11_SET_WINDOW_TITLE_WORKAROUND=1
    )
else()
    target_compile_definitions(
        dpscreenocr-qt
        PRIVATE
        DPSO_QT_X11_SET_WINDOW_TITLE_WORKAROUND=0
    )
endif()
