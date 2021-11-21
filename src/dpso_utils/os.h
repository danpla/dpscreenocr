
/**
 * \file
 * OS routines
 */

#pragma once

#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Directory separators for the current platform.
 *
 * The primary separator is first in the list.
 */
extern const char* const dpsoDirSeparators;


/**
 * fopen() wrapper that accepts fileName in UTF-8.
 */
FILE* dpsoFopenUtf8(const char* fileName, const char* mode);


#ifdef __cplusplus
}
#endif
