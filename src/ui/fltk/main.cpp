
#include <cstdio>
#include <cstring>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>

#include "dpso.h"

#include "hotkey_button.h"
//#include "language_choice.h"
#include "lang_browser.h"


static void resultCallback(const char* text, void* userData)
{
    (void)userData;

    const auto textLen = std::strlen(text);
    Fl::copy(text, textLen, 0);
    Fl::copy(text, textLen, 1);
}


int main(int argc, char **argv)
{
    dpsoInit();

    const DpsoCallbacks callbacks {nullptr, resultCallback, nullptr};
    dpsoSetCallbacks(&callbacks);

    dpsoSetHotheysEnabled(true);
    dpsoBindHotkey(dpsoKeyGrave, dpsoKeyModCtrl, 0);

    //Fl::add_idle(idleHandler, nullptr);
    //Fl::add_handler(globalShortcutHandler);

    auto* window = new Fl_Window(340, 180, "dpScreenOCR");

    // auto* langChoice = new LanguageChoice(
    //     12, 12 + 14, 316, 28, "Language:");
    // langChoice->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
    auto* langBrowser = new LangBrowser(
        12, 12 + 14, 316, 102, "Languages:");
    langBrowser->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
    window->resizable(langBrowser);

    auto* hotkeyButton = new HotkeyButton(12, 180 - 12 - 28, 316, 28, 0);

    window->end();
    window->show(argc, argv);

    //interactiveGrab();

    //const auto returnCode = Fl::run();
    while (Fl::first_window()) {
        if (dpsoGetSelectionIsEnabled()) {
            if (dpsoCatchHotkey() == 0) {
                dpsoSetSelectionIsEnabled(false);

                int x, y, w, h;
                dpsoGetSelectionGeometry(&x, &y, &w, &h);
                dpsoStartOcr(x, y, w, h);
                dpsoWaitForResult();
            } else
                dpsoUpdateSelection();
        } else if (dpsoCatchHotkey() == 0) {
            dpsoSetSelectionIsEnabled(true);
        }

        Fl::wait(1.0 / 60.0);
    }

    dpsoShutdown();

    //return returnCode;
    return 0;
}
