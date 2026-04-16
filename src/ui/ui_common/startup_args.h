#pragma once

#include <stdbool.h>


/**
 * Program startup arguments.
 */
typedef struct UiStartupArgs {
    /**
     * Start the program with the hidden window.
     */
    bool hide;
} UiStartupArgs;
