
#include "init.h"

#include "init_app_dirs.h"
#include "init_extra.h"
#include "init_intl.h"

#include "dpso_utils/error_get.h"
#include "dpso_utils/error_set.h"


bool uiInit(int argc, char* argv[])
{
    if (!ui::initStart(argc, argv)) {
        dpso::setError("ui::initStart: {}", dpsoGetError());
        return false;
    }

    if (!uiInitAppDirs(argv[0])) {
        dpso::setError("uiInitAppDirs: {}", dpsoGetError());
        return false;
    }

    ui::initIntl();

    if (!ui::initEnd(argc, argv)) {
        dpso::setError("ui::initEnd: {}", dpsoGetError());
        return false;
    }

    return true;
}
