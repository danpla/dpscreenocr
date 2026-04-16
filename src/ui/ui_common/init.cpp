#include "init.h"

#include "cmdline.h"
#include "exe_path.h"
#include "init_extra.h"
#include "init_intl.h"
#include "init_user_data.h"

#include "dpso_utils/error_get.h"
#include "dpso_utils/error_set.h"


bool uiInit(int argc, char* argv[], UiStartupArgs* startupArgs)
{
    if (!startupArgs) {
        dpso::setError("startupArgs is null");
        return false;
    }

    startupArgs = {};

    ui::processCmdLine(argc, argv, *startupArgs);

    if (!ui::initStart(argc, argv)) {
        dpso::setError("ui::initStart: {}", dpsoGetError());
        return false;
    }

    if (!ui::initExePath(argv[0])) {
        dpso::setError("ui::initExePath: {}", dpsoGetError());
        return false;
    }

    ui::initIntl();

    if (!ui::initUserData()) {
        dpso::setError("ui::initUserData: {}", dpsoGetError());
        return false;
    }

    if (!ui::initEnd(argc, argv, *startupArgs)) {
        dpso::setError("ui::initEnd: {}", dpsoGetError());
        return false;
    }

    return true;
}
