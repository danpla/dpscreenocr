
#pragma once

#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Perform platform-specific first time setup.
 *
 * The purpose of uiStartupSetup() is to perform any kinds of
 * platform-specific tasks that need to be done on program startup,
 * like setting up local user data, migrating files from previous
 * versions of the program, etc.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * false.
 */
bool uiStartupSetup(void);


#ifdef __cplusplus
}
#endif
