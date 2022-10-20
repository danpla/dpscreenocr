
set(DPSO_QT_VERSION "5" CACHE STRING "Qt version to use: 4, 5, or 6")
set_property(CACHE DPSO_QT_VERSION PROPERTY STRINGS 4 5 6)

set(SRCS

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
    list(APPEND SRCS $<TARGET_OBJECTS:windows_rc>)
endif()

add_executable(dpscreenocr_qt ${SRCS})

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    if(DPSO_QT_VERSION EQUAL 6)
        set(CPP_STANDARD 17)
    else()
        set(CPP_STANDARD 11)
    endif()

    target_compile_options(
        dpscreenocr_qt
        PRIVATE -std=c++${CPP_STANDARD} -Wall -Wextra -pedantic)

    unset(CPP_STANDARD)
endif()

target_include_directories(dpscreenocr_qt PRIVATE .)

set_target_properties(
    dpscreenocr_qt
    PROPERTIES
    AUTOMOC ON
    OUTPUT_NAME "dpscreenocr"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

if(WIN32 AND CMAKE_BUILD_TYPE MATCHES "Release|RelWithDebInfo|MinSizeRel")
    # Hide console window.
    set_target_properties(
        dpscreenocr_qt PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if(NOT TARGET dpso)
    add_subdirectory(../../dpso "${CMAKE_BINARY_DIR}/src/dpso")
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
    dpscreenocr_qt dpso dpso_intl dpso_utils ui_common)

if(DPSO_QT_VERSION EQUAL 4)
    find_package(Qt4 REQUIRED COMPONENTS QtCore QtGui)
    target_link_libraries(dpscreenocr_qt Qt4::QtCore Qt4::QtGui)
elseif(DPSO_QT_VERSION EQUAL 5 OR DPSO_QT_VERSION EQUAL 6)
    find_package(
        Qt${DPSO_QT_VERSION} REQUIRED COMPONENTS Core Gui Widgets)
    target_link_libraries(
        dpscreenocr_qt
        Qt${DPSO_QT_VERSION}::Core
        Qt${DPSO_QT_VERSION}::Gui
        Qt${DPSO_QT_VERSION}::Widgets)
else()
    message(FATAL_ERROR "Invalid Qt version \"${DPSO_QT_VERSION}\"")
endif()

# See https://bugreports.qt.io/browse/QTBUG-33775. This bug only
# affects Qt 5 < 5.3.2.
if(UNIX
        AND NOT APPLE
        AND DPSO_QT_VERSION EQUAL 5
        AND Qt5Widgets_VERSION VERSION_LESS "5.3.2")
    find_package(Qt5X11Extras REQUIRED)
    target_link_libraries(dpscreenocr_qt Qt5::X11Extras)

    find_package(X11 REQUIRED)
    target_include_directories(
        dpscreenocr_qt PRIVATE ${X11_INCLUDE_DIR})
    target_link_libraries(dpscreenocr_qt ${X11_LIBRARIES})
endif()
