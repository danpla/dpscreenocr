
# We no longer use CPack's NSIS generator due to lack of flexibility.
# For example:
#
# * It's impossible to remove the installed app silently.
#   CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL dialog not only allows
#   to skip uninstalling, but also uses hardcoded text that can't be
#   translated.
#
# * It's impossible to fix NSIS' InstallDirRegKey bug when installing
#   a 64-bit app from 32-bit installer (InstallDirRegKey always reads
#   32-bit view of registry in this case).

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(IS_64_BIT_BUILD 1)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(IS_64_BIT_BUILD 0)
endif()

configure_file(
    "${CMAKE_SOURCE_DIR}/data/nsis/nsis.nsi.in"
    "${CMAKE_BINARY_DIR}/nsis.nsi"
    @ONLY
)
file(
    WRITE
    "${CMAKE_BINARY_DIR}/nsis.nsi.readme.txt"
    "Build the \"prepare_nsis\" target to complete preparation and then run makensisw for nsis.nsi."
)

unset(IS_64_BIT_BUILD)

configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/dist_windows_nsis_prepare.cmake.in"
    "${CMAKE_BINARY_DIR}/dist_windows_nsis_prepare.cmake"
    @ONLY
)

add_custom_target(
    "prepare_nsis"
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_BINARY_DIR}/dist_windows_nsis_prepare.cmake"
    COMMAND ${CMAKE_COMMAND} -E echo "Run makensisw for nsis.nsi to build the installer"
    VERBATIM
)
