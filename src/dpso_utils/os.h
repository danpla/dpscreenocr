
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
 * Get file extension.
 *
 * The function returns a pointer to the period of the extension, or
 * null if filePath has no extension.
 */
const char* dpsoGetFileExt(const char* filePath);


/**
 * fopen() wrapper that accepts filePath in UTF-8.
 */
FILE* dpsoFopenUtf8(const char* filePath, const char* mode);


#ifdef __cplusplus
}
#endif
