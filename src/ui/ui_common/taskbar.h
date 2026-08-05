#pragma once

#include "taskbar_config.h"

#if UI_TASKBAR_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Taskbar handler.
 *
 * The taskbar handler allows to show a progress or error indicator on
 * a taskbar entry, usually known as a taskbar button. The "taskbar"
 * here is used as an abstract term; the implementation is free to
 * wrap any indicator (other than an interactive tray/status icon) on
 * a platform that doesn't use a classic Windows-style taskbar.
 *
 * Each platform platform-specific uiTaskbarCreate*() function should
 * return null and set an error message (dpsoGetError()) in case of
 * failure. For simplicity, we assume that an error when creating a
 * taskbar handler is normally not fatal for an application, so all
 * functions that accept a UiTaskbar pointer don't treat null as an
 * error. In particular, there's no special uiTaskbarCreate*() for
 * platforms on which the taskbar handler is not implemented: it's
 * supposed that UiTaskbar is explicitly set to null in this case.
 */
typedef struct UiTaskbar UiTaskbar;


#if UI_TASKBAR_WIN
UiTaskbar* uiTaskbarCreateWin(HWND hwnd);
#endif


void uiTaskbarDelete(UiTaskbar* tb);


typedef enum {
    /**
     * Normal, default taskbar state.
     */
    UiTaskbarStateNormal,

    /**
     * Progress.
     *
     * The taskbar indicates an indeterminate progress.
     */
    UiTaskbarStateProgress,

    /**
     * Error.
     *
     * The taskbar indicates an error, e.g. by turning red or showing
     * an overlay icon.
     */
    UiTaskbarStateError
} UiTaskbarState;


/**
 * Set taskbar state.
 *
 * The default state is UiTaskbarStateNormal.
 */
void uiTaskbarSetState(UiTaskbar* tb, UiTaskbarState newState);


#ifdef __cplusplus
}


#include <memory>


namespace ui {


struct TaskbarDeleter {
    void operator()(UiTaskbar* tb) const
    {
        uiTaskbarDelete(tb);
    }
};


using TaskbarUPtr = std::unique_ptr<UiTaskbar, TaskbarDeleter>;


}


#endif
