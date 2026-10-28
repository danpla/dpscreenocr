if(UNIX AND NOT APPLE)
    include(dist_unix_bundle)
elseif(WIN32)
    include(dist_windows_inno_setup)

    if(DPSO_ENABLE_MSIX)
        include(dist_windows_msix)
    endif()
endif()

# We don't set the project version in the project() call, so the
# CPACK_PACKAGE_VERSION_* variables are not set automatically.
set(CPACK_PACKAGE_VERSION_MAJOR "${APP_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${APP_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${APP_VERSION_PATCH}")

set(CPACK_STRIP_FILES YES)

include(CPack)
