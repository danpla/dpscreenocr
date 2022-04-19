
set(
    CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Program to recognize text on screen"
)

set(CPACK_PACKAGE_VERSION_MAJOR "${APP_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${APP_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${APP_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION "${APP_VERSION}")

set(CPACK_PACKAGE_VENDOR "${APP_AUTHOR}")

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.txt")

if(WIN32)
    include(dist_nsis)
    include(dist_wix)
endif()

configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/cpack_options.cmake.in"
    "${CMAKE_BINARY_DIR}/cpack_options.cmake" @ONLY
)
set(
    CPACK_PROJECT_CONFIG_FILE
    "${CMAKE_BINARY_DIR}/cpack_options.cmake"
)

include(CPack)
