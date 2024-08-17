
#pragma once

#include <stdbool.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef struct UiAutostart UiAutostart;


typedef struct UiAutostartArgs {
    /**
     * A human-readable application name.
     *
     * This name will be displayed in the system autostart manager.
     */
    const char* appName;

    /**
     * Application file name.
     *
     * This is a name of the application executable (without a file
     * name extension, if any) to be used on systems with file-based
     * autostart entries.
     */
    const char* appFileName;

    /**
     * Command line.
     *
     * The first item is mandatory and should be an absolute path to
     * the executable. The rest are its arguments, if any.
     */
    const char** args;
    size_t numArgs;
} UiAutostartArgs;


/**
 * Create an autostart handler.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * null.
 */
UiAutostart* uiAutostartCreate(const UiAutostartArgs* args);


void uiAutostartDelete(UiAutostart* autostart);


/**
 * Get whether autostart is enabled.
 *
 * UiAutostart is disabled by default, unless an active system
 * autostart entry already exists when uiAutostartCreate() is called.
 */
bool uiAutostartGetIsEnabled(const UiAutostart* autostart);


/**
 * Enable or disable autostart.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * false.
 */
bool uiAutostartSetIsEnabled(
    UiAutostart* autostart, bool newIsEnabled);


#ifdef __cplusplus
}


#include <memory>


namespace ui {


struct AutostartDeleter {
    void operator()(UiAutostart* autostart) const
    {
        uiAutostartDelete(autostart);
    }
};


using AutostartUPtr = std::unique_ptr<UiAutostart, AutostartDeleter>;


}


#endif
