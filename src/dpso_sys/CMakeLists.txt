cmake_minimum_required(VERSION 3.16)

project(dpso_sys)

add_library(
    dpso_sys

    dpso_sys.cpp
    key_manager.cpp
    keys.cpp
    screenshot.cpp
    selection.cpp)

# Backend
if(UNIX AND NOT APPLE)
    target_sources(
        dpso_sys
        PRIVATE
        backend/x11/backend.cpp
        backend/x11/key_manager.cpp
        backend/x11/screenshot.cpp
        backend/x11/selection.cpp)
elseif(WIN32)
    target_sources(
        dpso_sys
        PRIVATE
        backend/windows/execution_layer/action_executor.cpp
        backend/windows/execution_layer/backend_executor.cpp
        backend/windows/execution_layer/key_manager_executor.cpp
        backend/windows/execution_layer/selection_executor.cpp
        backend/windows/backend.cpp
        backend/windows/key_manager.cpp
        backend/windows/screenshot.cpp
        backend/windows/selection.cpp)
else()
    message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} is not supported")
endif()

target_compile_definitions(
    dpso_sys PRIVATE DPSO_DLL=$<BOOL:${BUILD_SHARED_LIBS}>)

set_target_properties(
    dpso_sys PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(dpso_sys PRIVATE -Wall -Wextra -pedantic)
endif()

target_include_directories(dpso_sys PRIVATE . PUBLIC ..)

if(NOT TARGET dpso_img)
    add_subdirectory(
        ../dpso_img "${CMAKE_BINARY_DIR}/src/dpso_img")
endif()

if(NOT TARGET dpso_utils)
    add_subdirectory(
        ../dpso_utils "${CMAKE_BINARY_DIR}/src/dpso_utils")
endif()

find_package(Threads REQUIRED)

target_link_libraries(
    dpso_sys
    PUBLIC dpso_img dpso_utils
    PRIVATE ${CMAKE_THREAD_LIBS_INIT})

if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    if(NOT X11_Xshape_FOUND)
        message(SEND_ERROR "X11 Shape Extension is not found")
    endif()

    target_include_directories(
        dpso_sys
        PRIVATE ${X11_INCLUDE_DIR} ${X11_Xshape_INCLUDE_PATH})
    target_link_libraries(
        dpso_sys PRIVATE ${X11_LIBRARIES} ${X11_Xext_LIB})
endif()
