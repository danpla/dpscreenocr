#pragma once

#include <cstddef>


extern "C" {


#if BUILDING_DLL
    #define EXPORT __declspec(dllexport)
#else
    #define EXPORT __declspec(dllimport)
#endif


EXPORT const char* MsixHelper_getLastError(std::size_t* len);

EXPORT bool MsixHelper_init();
EXPORT void MsixHelper_shutdown();

EXPORT bool MsixHelper_isActivatedByStartupTask();


typedef struct MsixHelper_StartupTask MsixHelper_StartupTask;

EXPORT MsixHelper_StartupTask*
MsixHelper_startupTaskCreate(const wchar_t* id);

EXPORT void MsixHelper_startupTaskDelete(MsixHelper_StartupTask* st);

EXPORT bool
MsixHelper_startupTaskGetIsEnabled(const MsixHelper_StartupTask* st);

EXPORT bool
MsixHelper_startupTaskSetIsEnabled(
    MsixHelper_StartupTask* st, bool newIsEnabled);


#undef EXPORT


}
