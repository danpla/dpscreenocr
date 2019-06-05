
set(OUT_RC_PATH "${CMAKE_CURRENT_BINARY_DIR}/resource.rc")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/resource.rc.in"
    "${OUT_RC_PATH}"
    @ONLY
)

add_library(windows_rc OBJECT "${OUT_RC_PATH}")
