#include "translation_preferred_langs.h"

#include <cstdlib>
#include <initializer_list>


namespace ui {


std::vector<std::string> getPreferredTranslationLangs()
{
    // See:
    // https://www.gnu.org/software/gettext/manual/html_node/Locale-Environment-Variables.html

    if (const auto* language = std::getenv("LANGUAGE");
            language && *language) {
        // LANGUAGE is the only variable that could be a list; the
        // others always contain a single locale name.
        std::vector<std::string> result;

        for (const auto* s = language; true;) {
            const auto* langBegin = s;

            while (*s && *s != ':')
                ++s;

            result.push_back({langBegin, s});
            if (!*s)
                break;

            ++s;
        }

        return result;
    }

    for (const auto* locale : {"LC_ALL", "LC_MESSAGES", "LANG"})
        if (locale && *locale)
            return {locale};

    return {};
}


}
