
#include "single_instance_guard.h"


struct UiSingleInstanceGuard {
};


UiSingleInstanceGuard* uiSingleInstanceGuardCreate(const char* id)
{
    (void)id;
    return new UiSingleInstanceGuard{};
}


void uiSingleInstanceGuardDelete(UiSingleInstanceGuard* guard)
{
    delete guard;
}


bool uiSingleInstanceGuardIsPrimary(
    const UiSingleInstanceGuard* guard)
{
    return guard != nullptr;
}
