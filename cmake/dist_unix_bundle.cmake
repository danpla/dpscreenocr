set(BUNDLE_BUILD_SCRIPT "${CMAKE_BINARY_DIR}/build_bundle.cmake")

configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/dist_unix_bundle_script.cmake.in"
    "${BUNDLE_BUILD_SCRIPT}"
    @ONLY)

string(TOLOWER "${CMAKE_SYSTEM_NAME}" OS_NAME)
set(BUNDLE_DIR_NAME
    "${APP_NAME}-${APP_VERSION}-${OS_NAME}-${CMAKE_SYSTEM_PROCESSOR}")

set(BUNDLE_BUILD_DIR "${CMAKE_BINARY_DIR}/bundle_build")
set(BUNDLE_DIR "${BUNDLE_BUILD_DIR}/${BUNDLE_DIR_NAME}")

# Note that we intentionally use targets that are always out of date,
# as CMake does not allow using the ALL target as a dependency.

add_custom_target(
    bundle
    COMMAND
        "${CMAKE_COMMAND}"
        -D "BUNDLE_BUILD_DIR=${BUNDLE_BUILD_DIR}"
        -D "BUNDLE_DIR=${BUNDLE_DIR}"
        -P "${BUNDLE_BUILD_SCRIPT}"
    VERBATIM)

set(BUNDLE_ARCHIVE_NAME "${BUNDLE_DIR_NAME}.tar.xz")

add_custom_target(
    bundle_archive
    COMMENT "Creating ${BUNDLE_ARCHIVE_NAME}"
    COMMAND
        "${CMAKE_COMMAND}" -E
        tar cJ
        "${CMAKE_BINARY_DIR}/${BUNDLE_ARCHIVE_NAME}"
        "${BUNDLE_DIR_NAME}"
    WORKING_DIRECTORY "${BUNDLE_BUILD_DIR}"
    VERBATIM)

add_dependencies(bundle_archive bundle)
