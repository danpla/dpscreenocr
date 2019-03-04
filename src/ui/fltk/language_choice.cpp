
#include "language_choice.h"

#include <cstdio>
#include <cstring>

#include "dpso.h"


static void changeLanguage(Fl_Widget* w, void* data)
{
    (void)data;

    auto* langChoice = static_cast<LanguageChoice*>(w);
    std::printf("Lang: %s\n", langChoice->text(langChoice->value()));
    dpsoSetActiveLanguage(langChoice->text(langChoice->value()));
}


LanguageChoice::LanguageChoice(
        int x, int y, int w, int h, const char* l)
    : Fl_Choice(x, y, w, h, l)
{
    int activeLangIdx = 0;
    const auto* activeLang = dpsoGetActiveLanguage();

    for (int i = 0, n = dpsoGetNumLanguages(); i < n; ++i) {
        const auto* lang = dpsoGetLanguage(i);
        if (std::strcmp(lang, activeLang) == 0)
            activeLangIdx = i;

        add(lang, 0, 0, 0, FL_MENU_TOGGLE);
    }

    value(activeLangIdx);

    callback(changeLanguage);
}
