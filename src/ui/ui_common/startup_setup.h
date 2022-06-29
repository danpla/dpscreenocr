
#pragma once


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Perform platform-specific first time setup.
 *
 * The purpose of startupSetup() is to perform any kinds of
 * platform-specific tasks that need to be done on program startup,
 * like setting up local user data, migrating files from previous
 * versions of the program, etc.
 */
int startupSetup(void);


#ifdef __cplusplus
}
#endif
