
#include "cmdline_cmd_autostart.h"

#include <cstdio>
#include <cstring>

#include "dpso_utils/error_get.h"
#include "dpso_utils/error_set.h"

#include "autostart.h"
#include "autostart_default_args.h"
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


bool cmdLineCmdAutostart(const char* argv0, const char* action)
{
    // uiAutostartGetDefaultArgs() uses the exe path.
    if (!ui::initExePath(argv0)) {
        dpso::setError("ui::initExePath(): {}", dpsoGetError());
        return false;
    }

    UiAutostartArgs autostartArgs;
    uiAutostartGetDefaultArgs(&autostartArgs);

    AutostartUPtr autostart{uiAutostartCreate(&autostartArgs)};
    if (!autostart) {
        dpso::setError(
            "Can't create autostart handler: {}",
            dpsoGetError());
        return false;
    }

    if (std::strcmp(action, "on") == 0)
        return setIsEnabled(autostart.get(), true);
    if (std::strcmp(action, "off") == 0)
        return setIsEnabled(autostart.get(), false);
    if (std::strcmp(action, "query") == 0) {
        std::printf(
            "%s\n",
            uiAutostartGetIsEnabled(autostart.get()) ? "on" : "off");
        return true;
    }

    dpso::setError("Unknown autostart action \"{}\"", action);
    return false;
}


}
