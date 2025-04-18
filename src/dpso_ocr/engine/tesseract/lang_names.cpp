#include "engine/tesseract/lang_names.h"

#include <algorithm>
#include <cstring>
#include <iterator>


namespace dpso::ocr::tesseract {
namespace {


struct LangInfo {
    const char* code;
    const char* name;
};


}


#define N_(S) S


// The list below is generated using scripts from
// tools/tessdata_info_gen for tessdata:3.04.00 and
// tessdata_fast:4.1.0. Please don't edit manually.
//
// The names are based on data from ISO 639-3 code tables:
// https://iso639-3.sil.org/code_tables/download_tables
//
// "equ" and "osd" are not included.
const LangInfo names[]{
    {"afr",          N_("Afrikaans")},
    {"amh",          N_("Amharic")},
    {"ara",          N_("Arabic")},
    {"asm",          N_("Assamese")},
    {"aze",          N_("Azerbaijani")},
    {"aze_cyrl",     N_("Azerbaijani (Cyrillic)")},
    {"bel",          N_("Belarusian")},
    {"ben",          N_("Bengali")},
    {"bod",          N_("Tibetan")},
    {"bos",          N_("Bosnian")},
    {"bre",          N_("Breton")},
    {"bul",          N_("Bulgarian")},
    {"cat",          N_("Catalan; Valencian")},
    {"ceb",          N_("Cebuano")},
    {"ces",          N_("Czech")},
    {"chi_sim",      N_("Chinese (simplified)")},
    {"chi_sim_vert", N_("Chinese (simplified, vertical)")},
    {"chi_tra",      N_("Chinese (traditional)")},
    {"chi_tra_vert", N_("Chinese (traditional, vertical)")},
    {"chr",          N_("Cherokee")},
    {"cos",          N_("Corsican")},
    {"cym",          N_("Welsh")},
    {"dan",          N_("Danish")},
    {"dan_frak",     N_("Danish (Fraktur)")},
    {"deu",          N_("German")},
    {"deu_frak",     N_("German (Fraktur)")},
    {"div",          N_("Dhivehi; Divehi; Maldivian")},
    {"dzo",          N_("Dzongkha")},
    {"ell",          N_("Greek, Modern (1453-)")},
    {"eng",          N_("English")},
    {"enm",          N_("English, Middle (1100-1500)")},
    {"epo",          N_("Esperanto")},
    {"est",          N_("Estonian")},
    {"eus",          N_("Basque")},
    {"fao",          N_("Faroese")},
    {"fas",          N_("Persian")},
    {"fil",          N_("Filipino; Pilipino")},
    {"fin",          N_("Finnish")},
    {"fra",          N_("French")},
    {"frk",          N_("German (Fraktur)")},
    {"frm",          N_("French, Middle (ca. 1400-1600)")},
    {"fry",          N_("Frisian, Western")},
    {"gla",          N_("Gaelic, Scottish")},
    {"gle",          N_("Irish")},
    {"glg",          N_("Galician")},
    {"grc",          N_("Greek, Ancient (to 1453)")},
    {"guj",          N_("Gujarati")},
    {"hat",          N_("Creole, Haitian")},
    {"heb",          N_("Hebrew")},
    {"hin",          N_("Hindi")},
    {"hrv",          N_("Croatian")},
    {"hun",          N_("Hungarian")},
    {"hye",          N_("Armenian")},
    {"iku",          N_("Inuktitut")},
    {"ind",          N_("Indonesian")},
    {"isl",          N_("Icelandic")},
    {"ita",          N_("Italian")},
    {"ita_old",      N_("Italian (old)")},
    {"jav",          N_("Javanese")},
    {"jpn",          N_("Japanese")},
    {"jpn_vert",     N_("Japanese (vertical)")},
    {"kan",          N_("Kannada")},
    {"kat",          N_("Georgian")},
    {"kat_old",      N_("Georgian (old)")},
    {"kaz",          N_("Kazakh")},
    {"khm",          N_("Khmer, Central")},
    {"kir",          N_("Kirghiz; Kyrgyz")},
    {"kmr",          N_("Kurdish, Northern")},
    {"kor",          N_("Korean")},
    {"kor_vert",     N_("Korean (vertical)")},
    {"kur",          N_("Kurdish")},
    {"lao",          N_("Lao")},
    {"lat",          N_("Latin")},
    {"lav",          N_("Latvian")},
    {"lit",          N_("Lithuanian")},
    {"ltz",          N_("Luxembourgish; Letzeburgesch")},
    {"mal",          N_("Malayalam")},
    {"mar",          N_("Marathi")},
    {"mkd",          N_("Macedonian")},
    {"mlt",          N_("Maltese")},
    {"mon",          N_("Mongolian")},
    {"mri",          N_("Maori")},
    {"msa",          N_("Malay")},
    {"mya",          N_("Burmese")},
    {"nep",          N_("Nepali")},
    {"nld",          N_("Dutch; Flemish")},
    {"nor",          N_("Norwegian")},
    {"oci",          N_("Occitan (post 1500)")},
    {"ori",          N_("Oriya")},
    {"pan",          N_("Panjabi; Punjabi")},
    {"pol",          N_("Polish")},
    {"por",          N_("Portuguese")},
    {"pus",          N_("Pushto; Pashto")},
    {"que",          N_("Quechua")},
    {"ron",          N_("Romanian; Moldavian; Moldovan")},
    {"rus",          N_("Russian")},
    {"san",          N_("Sanskrit")},
    {"sin",          N_("Sinhala; Sinhalese")},
    {"slk",          N_("Slovak")},
    {"slk_frak",     N_("Slovak (Fraktur)")},
    {"slv",          N_("Slovenian")},
    {"snd",          N_("Sindhi")},
    {"spa",          N_("Spanish; Castilian")},
    {"spa_old",      N_("Spanish; Castilian (old)")},
    {"sqi",          N_("Albanian")},
    {"srp",          N_("Serbian")},
    {"srp_latn",     N_("Serbian (Latin)")},
    {"sun",          N_("Sundanese")},
    {"swa",          N_("Swahili")},
    {"swe",          N_("Swedish")},
    {"syr",          N_("Syriac")},
    {"tam",          N_("Tamil")},
    {"tat",          N_("Tatar")},
    {"tel",          N_("Telugu")},
    {"tgk",          N_("Tajik")},
    {"tgl",          N_("Tagalog")},
    {"tha",          N_("Thai")},
    {"tir",          N_("Tigrinya")},
    {"ton",          N_("Tonga (Tonga Islands)")},
    {"tur",          N_("Turkish")},
    {"uig",          N_("Uighur; Uyghur")},
    {"ukr",          N_("Ukrainian")},
    {"urd",          N_("Urdu")},
    {"uzb",          N_("Uzbek")},
    {"uzb_cyrl",     N_("Uzbek (Cyrillic)")},
    {"vie",          N_("Vietnamese")},
    {"yid",          N_("Yiddish")},
    {"yor",          N_("Yoruba")},
};


const char* getLangName(const char* langCode)
{
    const auto iter = std::lower_bound(
        std::begin(names), std::end(names), langCode,
        [](const LangInfo& langInfo, const char* langCode)
        {
            return std::strcmp(langInfo.code, langCode) < 0;
        });

    if (iter != std::end(names)
            && std::strcmp(iter->code, langCode) == 0)
        return iter->name;

    return nullptr;
}


}
