find_program(MAKEPRI_EXE makepri REQUIRED)
find_program(MAKEAPPX_EXE makeappx REQUIRED)
find_program(SIGNTOOL_EXE signtool REQUIRED)

# APP_MSIX_ARCH will be used both for appxmanifest.xml and the MSIX
# file name. For valid values, see:
#
# https://learn.microsoft.com/en-us/uwp/schemas/appxpackage/uapmanifestschema/element-identity
#
# Note that we can't rely on CMAKE_SYSTEM_PROCESSOR. For Windows, it's
# documented to always return the value of the PROCESSOR_ARCHITECTURE
# environment variable. For example, if we build x86 binaries in the
# MinGW32 environment on a x64 machine, CMAKE_SYSTEM_PROCESSOR will
# return "AMD64" instead of "X86". As a workaround, we assume x86 when
# CMAKE_SIZEOF_VOID_P is 4, and fall back to CMAKE_SYSTEM_PROCESSOR in
# other cases.
#
# For possible values of PROCESSOR_ARCHITECTURE, see:
#
# https://learn.microsoft.com/en-us/windows/win32/winprog64/wow64-implementation-details#environment-variables
set(APP_MSIX_ARCH)
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(APP_MSIX_ARCH "x86")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
    set(APP_MSIX_ARCH "x64")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "X86")
    set(APP_MSIX_ARCH "x86")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "ARM64")
    set(APP_MSIX_ARCH "arm64")
else()
    message(
        FATAL_ERROR
        "Unexpected CMAKE_SYSTEM_PROCESSOR "
        "\"${CMAKE_SYSTEM_PROCESSOR}\"")
endif()

set(MSIX_BUILD_DIR "${CMAKE_BINARY_DIR}/msix_build")
set(MSIX_CONTENT_DIR "${MSIX_BUILD_DIR}/package_content")
set(MSIX_FILE
    "${CMAKE_BINARY_DIR}/${APP_NAME}-${APP_VERSION}-${APP_MSIX_ARCH}.msix")

configure_file(
    "${CMAKE_SOURCE_DIR}/dist/windows/msix/appxmanifest.xml.in"
    "${MSIX_BUILD_DIR}/appxmanifest.xml"
    @ONLY)

add_custom_target(
    msix
    COMMAND
        "${CMAKE_COMMAND}" -E rm -rf "${MSIX_CONTENT_DIR}"
    COMMAND
        "${CMAKE_COMMAND}"
        --build "${CMAKE_BINARY_DIR}"
        --parallel
    COMMAND
        "${CMAKE_COMMAND}"
        --install "${CMAKE_BINARY_DIR}"
        --strip
        --prefix "${MSIX_CONTENT_DIR}"
    COMMAND
        "${CMAKE_COMMAND}" -E copy
        "${MSIX_BUILD_DIR}/appxmanifest.xml"
        "${MSIX_CONTENT_DIR}"
    COMMAND
        "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_SOURCE_DIR}/dist/windows/msix/msix_assets"
        "${MSIX_CONTENT_DIR}/msix_assets"
    COMMAND
        "${CMAKE_COMMAND}"
        -D "DIR=${MSIX_CONTENT_DIR}/msix_assets"
        # The name of the ".resfiles" file should match the one from
        # priconfig.xml. Note that the path to this file specified in
        # priconfig.xml is computed relative to the /pr path passed to
        # makepri.exe.
        -D "RESFILES_PATH=${MSIX_BUILD_DIR}/priconfig.resfiles"
        -P "${CMAKE_CURRENT_LIST_DIR}/dist_windows_msix_gen_resfiles.cmake"
    COMMAND
        "${MAKEPRI_EXE}" new
        /pr "${MSIX_CONTENT_DIR}"
        /cf "${CMAKE_SOURCE_DIR}/dist/windows/msix/priconfig.xml"
        /of "${MSIX_CONTENT_DIR}/resources.pri"
    COMMAND
        "${MAKEAPPX_EXE}"
        pack /d "${MSIX_CONTENT_DIR}" /p "${MSIX_FILE}" /o
    COMMAND
        powershell
        -NoProfile -NonInteractive -ExecutionPolicy Bypass
        -File "${CMAKE_CURRENT_LIST_DIR}/dist_windows_msix_sign.ps1"
        -AppName "${APP_NAME}"
        -Publisher "${APP_MSIX_PUBLISHER}"
        -MsixPath "${MSIX_FILE}"
        -SigntoolPath "${SIGNTOOL_EXE}"
    VERBATIM)

set(MSIXUPLOAD_FILE "${MSIX_FILE}upload")

add_custom_target(
    msixupload
    COMMENT "Creating ${MSIXUPLOAD_FILE}"
    COMMAND
        "${CMAKE_COMMAND}" -E tar
        c "${MSIXUPLOAD_FILE}" --format=zip "${MSIX_FILE}"
    VERBATIM)

add_dependencies(msixupload msix)
