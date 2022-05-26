
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


/**
 * Synchronize file state with storage device.
 *
 * The function transfers all modified data (and possibly file
 * attributes) to the permanent storage device. It should normally be
 * preceded by fflush().
 *
 * On failure, sets an error message (dpsoGetError()) and returns 0.
 */
int dpsoSyncFile(FILE* fp);


/**
 * Synchronize directory containing filePath with storage device.
 *
 * The function is an equivalent of dpsoSyncFile() for directories:
 * it's usually called after creating a file to ensure that the new
 * directory entry has reached the storage device.
 *
 * On failure, sets an error message (dpsoGetError()) and returns 0.
 */
int dpsoSyncFileDir(const char* filePath);


#ifdef __cplusplus
}


#include <memory>


namespace dpso {


/**
 * Deleter for a FILE* smart pointer.
 */
struct StdFileCloser {
    void operator()(FILE* fp) const
    {
        if (fp)
            fclose(fp);
    }
};


using StdFileUPtr = std::unique_ptr<FILE, StdFileCloser>;


}


#endif
