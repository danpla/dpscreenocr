
#include "lang_name.h"

#include <algorithm>
#include <cstring>
#include <iterator>

#include "intl.h"


#define N_(S) S


namespace {


struct LangName {
    const char* code;
    const char* name;
};


}


// The names are from:
//   https://github.com/tesseract-ocr/tesseract/wiki/Data-Files-in-different-versions
//
// The entries must be sorted by the language code.
//
// "osd" is not included in the array.
// "kur" is the v3.04 name.
// "kur_ara" is renamed to "kmr" in the recent versions:
//   https://github.com/tesseract-ocr/langdata/issues/124
//
// Useful links:
//   https://en.wikipedia.org/wiki/List_of_ISO_639-3_codes
//   https://github.com/tesseract-ocr/tesseract/wiki/Data-Files
static const LangName names[] = {
    {"afr",      N_("Afrikaans")},
    {"amh",      N_("Amharic")},
    {"ara",      N_("Arabic")},
    {"asm",      N_("Assamese")},
    {"aze",      N_("Azerbaijani")},
    {"aze_cyrl", N_("Azerbaijani - Cyrilic")},
    {"bel",      N_("Belarusian")},
    {"ben",      N_("Bengali")},
    {"bod",      N_("Tibetan")},
    {"bos",      N_("Bosnian")},
    {"bre",      N_("Breton")},
    {"bul",      N_("Bulgarian")},
    {"cat",      N_("Catalan; Valencian")},
    {"ceb",      N_("Cebuano")},
    {"ces",      N_("Czech")},
    {"chi_sim",  N_("Chinese - Simplified")},
    {"chi_tra",  N_("Chinese - Traditional")},
    {"chr",      N_("Cherokee")},
    {"cym",      N_("Welsh")},
    {"dan",      N_("Danish")},
    {"dan_frak", N_("Danish - Fraktur")},
    {"deu",      N_("German")},
    {"deu_frak", N_("German - Fraktur")},
    {"dzo",      N_("Dzongkha")},
    {"ell",      N_("Greek, Modern (1453-)")},
    {"eng",      N_("English")},
    {"enm",      N_("English, Middle (1100-1500)")},
    {"epo",      N_("Esperanto")},
    {"equ",      N_("Math and equations")},
    {"est",      N_("Estonian")},
    {"eus",      N_("Basque")},
    {"fas",      N_("Persian")},
    {"fin",      N_("Finnish")},
    {"fra",      N_("French")},
    {"frk",      N_("German - Fraktur")},
    {"frm",      N_("French, Middle (ca.1400-1600)")},
    {"gle",      N_("Irish")},
    {"glg",      N_("Galician")},
    {"grc",      N_("Greek, Ancient (to 1453)")},
    {"guj",      N_("Gujarati")},
    {"hat",      N_("Haitian; Haitian Creole")},
    {"heb",      N_("Hebrew")},
    {"hin",      N_("Hindi")},
    {"hrv",      N_("Croatian")},
    {"hun",      N_("Hungarian")},
    {"iku",      N_("Inuktitut")},
    {"ind",      N_("Indonesian")},
    {"isl",      N_("Icelandic")},
    {"ita",      N_("Italian")},
    {"ita_old",  N_("Italian - Old")},
    {"jav",      N_("Javanese")},
    {"jpn",      N_("Japanese")},
    {"kan",      N_("Kannada")},
    {"kat",      N_("Georgian")},
    {"kat_old",  N_("Georgian - Old")},
    {"kaz",      N_("Kazakh")},
    {"khm",      N_("Central Khmer")},
    {"kir",      N_("Kirghiz; Kyrgyz")},
    {"kmr",      N_("Kurmanji (Kurdish - Latin Script)")},
    {"kor",      N_("Korean")},
    {"kor_vert", N_("Korean (vertical)")},
    {"kur",      N_("Kurdish (Arabic Script)")},
    {"kur_ara",  N_("Kurdish (Arabic Script)")},
    {"lao",      N_("Lao")},
    {"lat",      N_("Latin")},
    {"lav",      N_("Latvian")},
    {"lit",      N_("Lithuanian")},
    {"ltz",      N_("Luxembourgish")},
    {"mal",      N_("Malayalam")},
    {"mar",      N_("Marathi")},
    {"mkd",      N_("Macedonian")},
    {"mlt",      N_("Maltese")},
    {"mon",      N_("Mongolian")},
    {"mri",      N_("Maori")},
    {"msa",      N_("Malay")},
    {"mya",      N_("Burmese")},
    {"nep",      N_("Nepali")},
    {"nld",      N_("Dutch; Flemish")},
    {"nor",      N_("Norwegian")},
    {"oci",      N_("Occitan (post 1500)")},
    {"ori",      N_("Oriya")},
    {"pan",      N_("Panjabi; Punjabi")},
    {"pol",      N_("Polish")},
    {"por",      N_("Portuguese")},
    {"pus",      N_("Pushto; Pashto")},
    {"que",      N_("Quechua")},
    {"ron",      N_("Romanian; Moldavian; Moldovan")},
    {"rus",      N_("Russian")},
    {"san",      N_("Sanskrit")},
    {"sin",      N_("Sinhala; Sinhalese")},
    {"slk",      N_("Slovak")},
    {"slk_frak", N_("Slovak - Fraktur")},
    {"slv",      N_("Slovenian")},
    {"snd",      N_("Sindhi")},
    {"spa",      N_("Spanish; Castilian")},
    {"spa_old",  N_("Spanish; Castilian - Old")},
    {"sqi",      N_("Albanian")},
    {"srp",      N_("Serbian")},
    {"srp_latn", N_("Serbian - Latin")},
    {"sun",      N_("Sundanese")},
    {"swa",      N_("Swahili")},
    {"swe",      N_("Swedish")},
    {"syr",      N_("Syriac")},
    {"tam",      N_("Tamil")},
    {"tat",      N_("Tatar")},
    {"tel",      N_("Telugu")},
    {"tgk",      N_("Tajik")},
    {"tgl",      N_("Tagalog")},
    {"tha",      N_("Thai")},
    {"tir",      N_("Tigrinya")},
    {"ton",      N_("Tonga")},
    {"tur",      N_("Turkish")},
    {"uig",      N_("Uighur; Uyghur")},
    {"ukr",      N_("Ukrainian")},
    {"urd",      N_("Urdu")},
    {"uzb",      N_("Uzbek")},
    {"uzb_cyrl", N_("Uzbek - Cyrilic")},
    {"vie",      N_("Vietnamese")},
    {"yid",      N_("Yiddish")},
    {"yor",      N_("Yoruba")},
};


static bool cmpByCode(const LangName& name, const char* langCode)
{
    return std::strcmp(name.code, langCode) < 0;
}


const char* dpsoGetLangName(const char* langCode)
{
    const auto iter = std::lower_bound(
        std::begin(names), std::end(names), langCode, cmpByCode);

    if (iter != std::end(names)
            && std::strcmp(iter->code, langCode) == 0)
        return gettext(iter->name);

    return langCode;
}
