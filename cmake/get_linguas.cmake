
# Get the list of languages from po/LINGUAS.
function(get_linguas languages)
    file(
        STRINGS "${CMAKE_SOURCE_DIR}/po/LINGUAS" LANGS
        REGEX "^[^#].*"
    )

    set(${languages} ${LANGS} PARENT_SCOPE)
endfunction()
