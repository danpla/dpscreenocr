configure_file(manifest.xml.in manifest.xml @ONLY)

set(RC_DST_PATH "${CMAKE_CURRENT_BINARY_DIR}/resource.rc")

configure_file(resource.rc.in "${RC_DST_PATH}" @ONLY)

add_library(windows_rc OBJECT "${RC_DST_PATH}")
