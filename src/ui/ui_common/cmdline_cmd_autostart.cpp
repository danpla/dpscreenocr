#include "cmdline_cmd_autostart.h"

#include "dpso_utils/error_get.h"
#include "dpso_utils/error_set.h"
#include "dpso_utils/str_stdio.h"

#include "autostart.h"
#include "autostart_default.h"
#include "exe_path.h"


namespace ui {


static bool setIsEnabled(UiAutostart* autostart, bool isEnabled)
{
    if (uiAutostartSetIsEnabled(autostart, isEnabled))
        return true;

    dpso::setError(
        "Can't {} autostart: {}",
        isEnabled ? "enable" : "disable",
        dpsoGetError());
    return false;
}


bool cmdLineCmdAutostart(const char* argv0, std::string_view action)
{
    // uiAutostartCreateDefault() uses the exe path.
    if (!ui::initExePath(argv0)) {
        dpso::setError("ui::initExePath(): {}", dpsoGetError());
        return false;
    }

    AutostartUPtr autostart{uiAutostartCreateDefault()};
    if (!autostart) {
        dpso::setError(
            "Can't create autostart handler: {}",
            dpsoGetError());
        return false;
    }

    if (action == "on")
        return setIsEnabled(autostart.get(), true);
    if (action == "off")
        return setIsEnabled(autostart.get(), false);
    if (action == "query") {
        dpso::str::print(
            "{}\n",
            uiAutostartGetIsEnabled(autostart.get()) ? "on" : "off");
        return true;
    }

    dpso::setError("Unknown autostart action \"{}\"", action);
    return false;
}


}
