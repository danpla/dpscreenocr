#include "dpso_sys.h"

#include "dpso_utils/error_set.h"

#include "backend/backend.h"
#include "backend/backend_error.h"
#include "dpso_sys_fwd.h"
#include "dpso_sys_p.h"


DpsoSys* dpsoSysCreate(void)
{
    try {
        return new DpsoSys{};
    } catch (dpso::backend::BackendError& e) {
        dpso::setError("{}", e.what());
        return {};
    }
}


void dpsoSysDelete(DpsoSys* sys)
{
    delete sys;
}


void dpsoSysUpdate(DpsoSys* sys)
{
    if (sys)
        sys->backend->update();
}


DpsoKeyManager* dpsoSysGetKeyManager(DpsoSys* sys)
{
    return sys ? &sys->keyManager : nullptr;
}


DpsoSelection* dpsoSysGetSelection(DpsoSys* sys)
{
    return sys ? &sys->selection : nullptr;
}
