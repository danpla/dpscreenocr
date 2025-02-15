#pragma once

#include "dpso_sys_fwd.h"
#include "key_manager.h"
#include "screenshot.h"
#include "selection.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Create a system backend.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * null.
 */
DpsoSys* dpsoSysCreate(void);


void dpsoSysDelete(DpsoSys* sys);


/**
 * Update the system backend.
 *
 * The function process system events, like mouse motion, key press,
 * etc., for hotkey handling (key_manager.h) and interactive selection
 * (selection.h). Call this function at a frequency close to the
 * monitor refresh rate, which is usually 60 times per second.
 */
void dpsoSysUpdate(DpsoSys* sys);


DpsoKeyManager* dpsoSysGetKeyManager(DpsoSys* sys);
DpsoSelection* dpsoSysGetSelection(DpsoSys* sys);


#ifdef __cplusplus
}


#include <memory>


namespace dpso {


struct SysDeleter {
    void operator()(DpsoSys* sys) const
    {
        dpsoSysDelete(sys);
    }
};


using SysUPtr = std::unique_ptr<DpsoSys, SysDeleter>;


}


#endif
