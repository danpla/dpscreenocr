#pragma once

#include <stdbool.h>


/**
 * Program startup arguments from the command line options.
 */
typedef struct UiStartupArgs {
    /**
     * Start the program with the hidden window.
     */
    bool hide;
} UiStartupArgs;
