
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(APP_IS_64_BIT 1)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(APP_IS_64_BIT 0)
else()
    message(
        FATAL_ERROR
        "Unexpected CMAKE_SIZEOF_VOID_P (${CMAKE_SIZEOF_VOID_P})"
    )
endif()

string(REPLACE "/" "\\" APP_SOURCE_DIR "${CMAKE_SOURCE_DIR}")

configure_file(
    "${CMAKE_SOURCE_DIR}/data/iss/inno_setup.iss.in"
    "${CMAKE_BINARY_DIR}/inno_setup.iss"
    @ONLY
)

unset(APP_IS_64_BIT)
unset(APP_SOURCE_DIR)

# Generate inno_setup_languages.isi.
function(gen_language_list)
    # This is the mapping from a locale name in po/LINGUAS to a name
    # if the Inno Setup language file from "compiler:Languages\".
    # English is always included and is not listed here. To skip a
    # locale that has no corresponding language file, use - as the
    # file name, e.g.:
    #
    #   set(ISL_en_GB "-")
    #
    # Inno Setup is only shipped with official languages. You can
    # download the unofficial ones either from the Inno Setup source
    # code repository (Files/Languages/Unofficial/) or from
    # https://jrsoftware.org/files/istrans/. The list below expects
    # the unofficial languages to be in the "Unofficial" subdirectory,
    # i.e. "compiler:Languages\Unofficial\".
    set(ISL_bg "Bulgarian")
    set(ISL_ca "Catalan")
    set(ISL_de "German")
    set(ISL_es "Spanish")
    set(ISL_fr "French")
    set(ISL_hr "Unofficial\\Croatian")
    set(ISL_nb_NO "Norwegian")
    set(ISL_pl "Polish")
    set(ISL_ru "Russian")
    set(ISL_tr "Turkish")
    set(ISL_uk "Ukrainian")
    set(ISL_zh_CN "Unofficial\\ChineseSimplified")

    set(OUT_FILE "${CMAKE_BINARY_DIR}/inno_setup_languages.isi")

    if(NOT DPSO_ENABLE_NLS)
        file(
            WRITE
            "${OUT_FILE}"
            "; No languages since NLS was disabled"
        )
        return()
    endif()

    file(REMOVE "${OUT_FILE}")

    set(UNDEFINED_ISLS)

    include(get_linguas)
    get_linguas(LANGS)
    foreach(LANG ${LANGS})
        if(NOT ISL_${LANG})
            list(APPEND UNDEFINED_ISLS "${LANG}")
        elseif(NOT ISL_${LANG} STREQUAL "-")
            file(
                APPEND
                "${OUT_FILE}"
                "Name: \"${LANG}\"; MessagesFile: \"compiler:Languages\\${ISL_${LANG}}.isl\"\n"
            )
        endif()
    endforeach()

    if(UNDEFINED_ISLS)
        string(
            REPLACE ";" ", " UNDEFINED_ISLS_STR "${UNDEFINED_ISLS}"
        )
        message(
            WARNING
            "Inno Setup language files for the following locales are not defined: ${UNDEFINED_ISLS_STR}. Add them to the list above."
        )
    endif()
endfunction()

gen_language_list()
