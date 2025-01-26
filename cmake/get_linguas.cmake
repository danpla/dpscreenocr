# Get the list of languages from po/LINGUAS.
function(get_linguas LANGUAGES)
    file(
        STRINGS "${CMAKE_SOURCE_DIR}/po/LINGUAS" LANGS
        REGEX "^[^#].*")

    set(${LANGUAGES} ${LANGS} PARENT_SCOPE)
endfunction()
