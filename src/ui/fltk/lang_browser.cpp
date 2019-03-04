
#include "lang_browser.h"

#include <cstdio>
#include <cstring>

#include "dpso.h"


static void toggleLang(Fl_Widget* w, void* data)
{
    (void)data;

    auto* browser = static_cast<LangBrowser*>(w);

    std::printf(
        "Lang toggle: %s %i %i\n",
        browser->text(browser->value()),
        browser->checked(browser->value()),
        browser->value());
    const auto value = browser->value();
    const auto langIdx = value - 1;
    dpsoSetLanguageIsActive(langIdx, browser->checked(value));
    //dpsoSetActiveLanguage(langChoice->text(langChoice->value()));
}


LangBrowser::LangBrowser(
        int x, int y, int w, int h, const char* l)
    : Fl_Check_Browser(x, y, w, h, l)
{
    when(FL_WHEN_CHANGED);

    for (int i = 0, n = dpsoGetNumLanguages(); i < n; ++i) {
        const auto* langCode = dpsoGetLanguageCode(i);
        const auto state = dpsoGetLanguageIsActive(i);
        add(langCode, state);
    }

    //value(activeLangIdx);

    callback(toggleLang);

}
