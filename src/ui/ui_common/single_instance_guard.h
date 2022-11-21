
#pragma once

#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * A single instance guard allows to check whether the application is
 * a primary or a secondary instance.
 */
typedef struct UiSingleInstanceGuard UiSingleInstanceGuard;


/**
 * Create a single instance guard.
 *
 * Id is a unique identifier of the application (like application name
 * or UUID) that should be the same for all instances.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * null.
 */
UiSingleInstanceGuard* uiSingleInstanceGuardCreate(const char* id);


void uiSingleInstanceGuardDelete(UiSingleInstanceGuard* guard);


bool uiSingleInstanceGuardIsPrimary(
    const UiSingleInstanceGuard* guard);


#ifdef __cplusplus
}


#include <memory>


namespace ui {


struct SingleInstanceGuardDeleter {
    void operator()(UiSingleInstanceGuard* guard) const
    {
        uiSingleInstanceGuardDelete(guard);
    }
};


using SingleInstanceGuardUPtr =
    std::unique_ptr<
        UiSingleInstanceGuard, SingleInstanceGuardDeleter>;


}


#endif
