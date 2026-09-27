add_custom_target(
    uninstall
    COMMAND
        "${CMAKE_COMMAND}"
        -D "INSTALL_MANIFEST_PATH=${CMAKE_BINARY_DIR}/install_manifest.txt"
        -P "${CMAKE_CURRENT_LIST_DIR}/uninstall_unix_script.cmake"
    VERBATIM)
