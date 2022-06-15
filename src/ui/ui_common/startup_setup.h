
#pragma once


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Perform platform-specific first time setup.
 *
 * The purpose of startupSetup() is to setup local user data, migrate
 * files from previous version of the program, etc. Normally, this
 * function operates on user paths from dpsoGetUserDir(), or on the
 * directory of the executable if portableMode is nonzero.
 */
int startupSetup(int portableMode);


#ifdef __cplusplus
}
#endif
