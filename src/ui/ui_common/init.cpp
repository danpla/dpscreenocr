#include "init.h"

#include "exe_path.h"
#include "init_app_dirs.h"
#include "init_extra.h"
#include "init_intl.h"
#include "init_startup_args.h"

#include "dpso_utils/error_get.h"
#include "dpso_utils/error_set.h"


bool uiInit(int argc, char* argv[], UiStartupArgs* startupArgs)
{
    if (!startupArgs) {
        dpso::setError("startupArgs is null");
        return false;
    }

    *startupArgs = ui::initStartupArgs(argc, argv);

    if (!ui::initStart(argc, argv)) {
        dpso::setError("ui::initStart: {}", dpsoGetError());
        return false;
    }

    if (!ui::initExePath(argv[0])) {
        dpso::setError("ui::initExePath: {}", dpsoGetError());
        return false;
    }

    if (!ui::initAppDirs()) {
        dpso::setError("ui::initAppDirs: {}", dpsoGetError());
        return false;
    }

    ui::initIntl();

    if (!ui::initEnd(argc, argv)) {
        dpso::setError("ui::initEnd: {}", dpsoGetError());
        return false;
    }

    return true;
}
