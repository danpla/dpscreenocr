cmake_minimum_required(VERSION 2.8.12)

project(dpso)

option(
    DPSO_FORCE_TIMING
    "Force code timing (dpso::CodeTimer) even in release mode"
    NO
)

set(BACKEND_SRCS)
if(UNIX AND NOT APPLE)
    list(
        APPEND BACKEND_SRCS
        backend/x11/x11_backend.cpp
        backend/x11/x11_key_manager.cpp
        backend/x11/x11_screenshot.cpp
        backend/x11/x11_selection.cpp
    )
elseif(WIN32)
    list(
        APPEND BACKEND_SRCS
        backend/windows/execution_layer/action_executor.cpp
        backend/windows/execution_layer/backend_executor.cpp
        backend/windows/execution_layer/key_manager_executor.cpp
        backend/windows/execution_layer/selection_executor.cpp
        backend/windows/utils.cpp
        backend/windows/windows_backend.cpp
        backend/windows/windows_key_manager.cpp
        backend/windows/windows_screenshot.cpp
        backend/windows/windows_selection.cpp
    )
else()
    message(FATAL_ERROR "${CMAKE_SYSTEM_NAME} is not supported")
endif()

add_library(
    dpso

    delay.cpp
    dpso.cpp
    error.cpp
    geometry.cpp
    geometry_c.cpp
    hotkeys.cpp
    img.cpp
    key_names.cpp
    ocr.cpp
    ocr_engine/ocr_engine.cpp
    ocr_engine/ocr_result.cpp
    ocr_engine/ocr_result_text.cpp
    ocr_engine/tesseract/lang_names.cpp
    ocr_engine/tesseract/tesseract_ocr_engine.cpp
    ocr_engine/tesseract/utils.cpp
    progress_tracker.cpp
    selection.cpp
    str.cpp

    ${BACKEND_SRCS}
)

target_compile_definitions(
    dpso PRIVATE DPSO_FORCE_TIMING=$<BOOL:${DPSO_FORCE_TIMING}>
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
        OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
        dpso PRIVATE -std=c++11 -Wall -Wextra -pedantic
    )
endif()

target_include_directories(dpso PRIVATE . PUBLIC ..)

add_subdirectory(
    ../thirdparty/stb_image_resize
    ${CMAKE_CURRENT_BINARY_DIR}/src/thirdparty/stb_image_resize
)
target_link_libraries(dpso stb_image_resize)

set(CMAKE_THREAD_PREFER_PTHREAD True)
find_package(Threads REQUIRED)
if(CMAKE_USE_PTHREADS_INIT)
    target_compile_options(dpso PRIVATE -pthread)
endif()
target_link_libraries(dpso ${CMAKE_THREAD_LIBS_INIT})

if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    if(NOT X11_Xshape_FOUND)
        message(SEND_ERROR "X11 Shape Extension is not found")
    endif()

    target_include_directories(
        dpso PRIVATE ${X11_INCLUDE_DIR} ${X11_Xshape_INCLUDE_PATH}
    )
    target_link_libraries(dpso ${X11_LIBRARIES} ${X11_Xext_LIB})
endif()

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_search_module(TESSERACT tesseract)
endif()

if(NOT TESSERACT_FOUND)
    # No pkg-config or an old Tesseract version that doesn't provide
    # a .pc file.
    if(NOT EXISTS "/usr/include/tesseract")
        message(SEND_ERROR "Tesseract is not found")
    endif()
    set(TESSERACT_INCLUDE_DIRS /usr/include)
    set(TESSERACT_LIBRARIES tesseract)
endif()

target_include_directories(dpso PRIVATE ${TESSERACT_INCLUDE_DIRS})
target_link_libraries(dpso ${TESSERACT_LIBRARIES})
