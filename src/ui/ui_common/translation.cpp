#include "translation.h"

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "dp_intl/file_data_stream.h"
#include "dp_intl/lang_match.h"
#include "dp_intl/lang_tag_utils.h"
#include "dp_intl/translator.h"
#include "dp_intl/translator_plural_spec.h"

#include "dpso_utils/error_set.h"
#include "dpso_utils/str.h"
#include "dpso_utils/os.h"

#include "app_dirs.h"
#include "translation_init.h"
#include "translation_preferred_langs.h"


using namespace dpso;


namespace {


class Error : public std::runtime_error {
    using runtime_error::runtime_error;
};


using Translator = dp::intl::Translator<dp::intl::GermanicPluralSpec>;

Translator tr;
std::string trLang{"en"};


// Throws Error.
void pickTranslationLang(const std::vector<std::string>& langTags)
{
    tr = Translator{};
    trLang = "en";

    dp::intl::matchLang(
        langTags,
        [](const std::vector<std::string_view>& subtags)
        {
            const auto langTag = dp::intl::buildLangTag(subtags);
            if (langTag == "en")
                return true;

            namespace fs = std::filesystem;

            const auto moPath = fs::u8path(uiGetAppDir(UiAppDirData))
                / fs::u8path("translations")
                / fs::u8path(langTag + ".mo");

            std::optional<dp::intl::FileDataStream> file;
            try {
                file.emplace(moPath);
            } catch (dp::intl::FileNotFoundError&) {
                return false;
            } catch (dp::intl::Error& e) {
                throw Error{str::format(
                    "dp::intl::FileDataStream{\"{}\"}: {}",
                    moPath.u8string(), e.what())};
            }

            try {
                tr = Translator{*file};
            } catch (dp::intl::Error& e) {
                throw Error{str::format(
                    "dp::intl::Translator for \"{}\": {}",
                    moPath.u8string(), e.what())};
            }

            trLang = langTag;
            return true;
        });
}


}


namespace ui {


bool initTranslation()
{
    std::vector<std::string> tags;

    // Since our application currently doesn't have a language
    // selection menu, we have an internal "DPSO_LANG" environment
    // variable as a quick way to force the language without messing
    // with OS settings, in case we need to take screenshots of the
    // application in different languages.
    if (const auto* dpsoLang = std::getenv("DPSO_LANG"))
        tags.push_back(dpsoLang);
    else
        tags = ui::getPreferredTranslationLangs();

    try {
        pickTranslationLang(tags);
    } catch (Error& e) {
        setError("{}", e.what());
        return false;
    }

    return true;
}


}


const char* uiGetTranslationLang(void)
{
    return trLang.c_str();
}


const char* uiTranslate(const char* id)
{
    return tr.translateCStr(id);
}


const char* uiTranslateContext(const char* context, const char* id)
{
    return tr.translateContextCStr(context, id);
}


const char* uiTranslateN(
    uint64_t n, const char* idSingular, const char* idPlural)
{
    return tr.translateNCStr(n, idSingular, idPlural);
}
