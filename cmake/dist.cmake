if(UNIX AND NOT APPLE)
    include(dist_unix_bundle)
elseif(WIN32)
    include(dist_windows_inno_setup)

    if(DPSO_ENABLE_MSIX)
        include(dist_windows_msix)
    endif()
endif()

# Since we only use CPack to create archives, we only need to set the
# version variables (we don't set the project version in the project()
# call, so the CPACK_PACKAGE_VERSION_* variables are not set
# automatically).

set(CPACK_PACKAGE_VERSION_MAJOR "${APP_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${APP_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${APP_VERSION_PATCH}")

include(CPack)
