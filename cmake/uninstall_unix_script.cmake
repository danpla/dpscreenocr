# The script requires the INSTALL_MANIFEST_PATH option pointing
# to the "install_manifest.txt" file.

if(NOT EXISTS "${INSTALL_MANIFEST_PATH}")
    message(FATAL_ERROR "\"${INSTALL_MANIFEST_PATH}\" does not exist")
endif()

file(STRINGS "${INSTALL_MANIFEST_PATH}" FILES)

foreach(FILE ${FILES})
    set(FILE_PATH "$ENV{DESTDIR}${FILE}")

    if(NOT IS_SYMLINK "${FILE_PATH}" AND NOT EXISTS "${FILE_PATH}")
        message(STATUS "Does not exist: ${FILE_PATH}")
        continue()
    endif()

    message(STATUS "Uninstalling: ${FILE_PATH}")
    file(REMOVE "${FILE_PATH}")
endforeach()
