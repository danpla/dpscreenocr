
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/manifest.xml.in"
    "${CMAKE_CURRENT_BINARY_DIR}/manifest.xml"
    @ONLY
)

set(RC_DST_PATH "${CMAKE_CURRENT_BINARY_DIR}/resource.rc")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/resource.rc.in"
    "${RC_DST_PATH}"
    @ONLY
)

add_library(windows_rc OBJECT "${RC_DST_PATH}")
