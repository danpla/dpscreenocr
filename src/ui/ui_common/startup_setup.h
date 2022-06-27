
#pragma once


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Perform platform-specific first time setup.
 *
 * The purpose of startupSetup() is to setup local user data, migrate
 * files from previous version of the program, etc. Normally, this
 * function operates on paths from dpsoGetUserDir().
 */
int startupSetup(void);


#ifdef __cplusplus
}
#endif
