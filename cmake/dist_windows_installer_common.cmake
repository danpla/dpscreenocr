
# We use "Required" as the default install component.
#
# It's possible to completely ignore the COMPONENT for default
# install() commands, because the default ("Unspecified") component is
# always hidden and required. However, it doesn't play well with
# interfaces of installers:
#
#   * NSIS: It's seems a common practice to show the main program
#     component, which is always enabled.
#
#   * WIX (components are called features): The GUI will show the
#     wrong size for the root feature if there is no explicit
#     sub-feature that actually includes all files of install()
#     commands.
#
# The components in NSIS installer are sorted by CMake component name.
# To force the main component to be the first in the list, we start it
# with a capital letter; by convention, all other component names
# start with a lower-case letter.
#
# We can't use "default" as a component name, because NSIS gives an
# error in this case.

set(CPACK_COMPONENT_REQUIRED_DISPLAY_NAME "${APP_NAME}")
set(CPACK_COMPONENT_REQUIRED_REQUIRED TRUE)
set(CPACK_COMPONENT_REQUIRED_DESCRIPTION "${APP_NAME} ${APP_VERSION}")

set(CPACK_COMPONENT_LOCALIZATION_DISPLAY_NAME "Translations")
set(
    CPACK_COMPONENT_LOCALIZATION_DESCRIPTION
    "Translations for the user interface. Without this, ${APP_NAME} will only be available in English."
)

set(CPACK_PACKAGE_EXECUTABLES "${APP_FILE_NAME}" "${APP_NAME}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${APP_NAME}")
