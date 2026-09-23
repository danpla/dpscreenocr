function(iss_gen_setup_config OUT_DIR)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(APP_IS_64_BIT Yes)
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(APP_IS_64_BIT No)
    else()
        message(
            FATAL_ERROR
            "Unexpected CMAKE_SIZEOF_VOID_P (${CMAKE_SIZEOF_VOID_P})")
    endif()

    string(REPLACE "/" "\\" APP_SOURCE_DIR "${CMAKE_SOURCE_DIR}")

    configure_file(
        "${CMAKE_SOURCE_DIR}/dist/windows/iss/inno_setup_config.isi.in"
        "${OUT_DIR}/inno_setup_config.isi"
        @ONLY)
endfunction()

function(iss_gen_language_list OUT_DIR)
    # This is the mapping from a language code in po/LINGUAS to a name
    # if the Inno Setup language file from "compiler:Languages\".
    # English is always included and is not listed here. To skip a
    # code that has no corresponding language file, use - as the
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
    set(ISL_he "Hebrew")
    set(ISL_hr "Unofficial\\Croatian")
    set(ISL_it "Italian")
    set(ISL_nb_NO "Norwegian")
    set(ISL_pl "Polish")
    set(ISL_pt_BR "BrazilianPortuguese")
    set(ISL_ru "Russian")
    set(ISL_tr "Turkish")
    set(ISL_uk "Ukrainian")
    set(ISL_zh_CN "Unofficial\\ChineseSimplified")

    set(CONTENT "")
    set(UNDEFINED_ISLS)

    include(get_linguas)
    get_linguas(LANGS)
    foreach(LANG ${LANGS})
        if(NOT ISL_${LANG})
            list(APPEND UNDEFINED_ISLS "${LANG}")
            string(
                APPEND
                CONTENT
                "; Name: \"${LANG}\"; MessagesFile: Not found\n")
        elseif(NOT ISL_${LANG} STREQUAL "-")
            string(
                APPEND
                CONTENT
                "Name: \"${LANG}\"; MessagesFile: \"compiler:Languages\\${ISL_${LANG}}.isl\"\n")
        endif()
    endforeach()

    file(
        GENERATE
        OUTPUT "${OUT_DIR}/inno_setup_languages.isi"
        CONTENT "${CONTENT}")

    if(UNDEFINED_ISLS)
        string(
            REPLACE ";" ", " UNDEFINED_ISLS_STR "${UNDEFINED_ISLS}")
        message(
            WARNING
            "Inno Setup language files for the following languages "
            "are not defined: ${UNDEFINED_ISLS_STR}. Add them to the "
            "list above.")
    endif()
endfunction()

set(ISS_BUILD_DIR "${CMAKE_BINARY_DIR}/iss_build")
set(ISS_APP_DIR "${ISS_BUILD_DIR}/${APP_FILE_NAME}")

set(ISS_SCRIPT "${ISS_BUILD_DIR}/inno_setup.iss")
configure_file(
    "${CMAKE_SOURCE_DIR}/dist/windows/iss/inno_setup.iss"
    "${ISS_SCRIPT}"
    COPYONLY)

iss_gen_setup_config("${ISS_BUILD_DIR}")
iss_gen_language_list("${ISS_BUILD_DIR}")

add_custom_target(
    iss
    COMMAND
        "${CMAKE_COMMAND}" -E rm -rf "${ISS_APP_DIR}"
    COMMAND
        "${CMAKE_COMMAND}"
        --build "${CMAKE_BINARY_DIR}"
        --parallel
    COMMAND
        "${CMAKE_COMMAND}"
        --install "${CMAKE_BINARY_DIR}"
        --strip
        --prefix "${ISS_APP_DIR}"
    COMMAND
        "${CMAKE_COMMAND}" -E echo
        "You can now build the installer using \"${ISS_SCRIPT}\""
    VERBATIM)
