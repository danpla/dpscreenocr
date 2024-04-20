
# Generate inno_setup_config.isi.
function(gen_inno_setup_config)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(APP_IS_64_BIT Yes)
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(APP_IS_64_BIT No)
    else()
        message(
            FATAL_ERROR
            "Unexpected CMAKE_SIZEOF_VOID_P (${CMAKE_SIZEOF_VOID_P})")
    endif()

    set(APP_UI "${DPSO_UI}")
    if(DPSO_UI STREQUAL "qt")
        set(APP_UI "${APP_UI}${DPSO_QT_VERSION}")
    endif()

    set(APP_TESSERACT_VERSION_MAJOR "${DPSO_TESSERACT_VERSION_MAJOR}")

    string(REPLACE "/" "\\" APP_SOURCE_DIR "${CMAKE_SOURCE_DIR}")

    configure_file(
        "${CMAKE_SOURCE_DIR}/dist/windows/iss/inno_setup_config.isi.in"
        "${CMAKE_BINARY_DIR}/inno_setup_config.isi"
        @ONLY)
endfunction()

# Generate inno_setup_languages.isi.
function(gen_inno_setup_language_list)
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
    set(ISL_pt_BR "BrazilianPortuguese")
    set(ISL_ru "Russian")
    set(ISL_tr "Turkish")
    set(ISL_uk "Ukrainian")
    set(ISL_zh_CN "Unofficial\\ChineseSimplified")

    set(OUT_FILE "${CMAKE_BINARY_DIR}/inno_setup_languages.isi")

    file(REMOVE "${OUT_FILE}")

    set(UNDEFINED_ISLS)

    include(get_linguas)
    get_linguas(LANGS)
    foreach(LANG ${LANGS})
        if(NOT ISL_${LANG})
            list(APPEND UNDEFINED_ISLS "${LANG}")
            file(
                APPEND
                "${OUT_FILE}"
                "; Name: \"${LANG}\"; MessagesFile: Not found\n")
        elseif(NOT ISL_${LANG} STREQUAL "-")
            file(
                APPEND
                "${OUT_FILE}"
                "Name: \"${LANG}\"; MessagesFile: \"compiler:Languages\\${ISL_${LANG}}.isl\"\n")
        endif()
    endforeach()

    if(UNDEFINED_ISLS)
        string(
            REPLACE ";" ", " UNDEFINED_ISLS_STR "${UNDEFINED_ISLS}")
        message(
            WARNING
            "Inno Setup language files for the following locales are "
            "not defined: ${UNDEFINED_ISLS_STR}. Add them to the "
            "list above.")
    endif()
endfunction()

configure_file(
    "${CMAKE_SOURCE_DIR}/dist/windows/iss/inno_setup.iss"
    "${CMAKE_BINARY_DIR}/inno_setup.iss"
    COPYONLY)

gen_inno_setup_config()
gen_inno_setup_language_list()
