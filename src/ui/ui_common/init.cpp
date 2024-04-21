
#include "init.h"

#include "init_app_dirs.h"
#include "init_environment.h"
#include "init_intl.h"

#include "dpso_utils/error_get.h"
#include "dpso_utils/error_set.h"


bool uiInit(int argc, char* argv[])
{
    (void)argc;

    if (!uiInitEnvironment(argv)) {
        dpso::setError("uiInitEnvironment: {}", dpsoGetError());
        return false;
    }

    if (!uiInitAppDirs(argv[0])) {
        dpso::setError("uiInitAppDirs: {}", dpsoGetError());
        return false;
    }

    uiInitIntl();

    return true;
}
