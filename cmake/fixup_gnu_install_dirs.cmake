
# By default, CMAKE_INSTALL_DOCDIR uses PROJECT_NAME, which is in
# title case. We don't depend on PROJECT_NAME and use APP_FILE_NAME
# instead.
#
# Note that we only overwrite the default CMAKE_INSTALL_DOCDIR, i.e.
# when it's not set explicitly.

if(UNIX AND NOT APPLE AND NOT CMAKE_INSTALL_DOCDIR)
    include(GNUInstallDirs)
    set(
        CMAKE_INSTALL_DOCDIR
        "${CMAKE_INSTALL_DATADIR}/doc/${APP_FILE_NAME}"
    )
endif()
