# Convert line endings to NEWLINE_STYLE, which can be any value
# accepted by the same option of file(GENERATE).
function(convert_line_endings IN_FILE OUT_FILE NEWLINE_STYLE)
    file(
        GENERATE
        OUTPUT "${OUT_FILE}"
        INPUT "${IN_FILE}"
        NEWLINE_STYLE "${NEWLINE_STYLE}")
endfunction()
