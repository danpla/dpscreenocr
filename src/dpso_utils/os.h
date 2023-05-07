
/**
 * \file
 * OS routines
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Block the current thread for the given number of milliseconds.
 */
void dpsoSleep(int milliseconds);


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
 * Get file size.
 *
 * The behavior is platform-specific if the filePath points to
 * anything other than a regular file. On failure, sets an error
 * message (dpsoGetError()) and returns -1.
 */
int64_t dpsoGetFileSize(const char* filePath);


/**
 * fopen() that accepts filePath in UTF-8.
 */
FILE* dpsoFopen(const char* filePath, const char* mode);


/**
 * remove() that accepts filePath in UTF-8.
 */
int dpsoRemove(const char* filePath);


/**
 * Rename a file or directory, replacing destination.
 *
 * Unlike std::rename(), this function silently replaces an existing
 * dst on all platforms. On failure, sets an error message
 * (dpsoGetError()) and returns false.
 */
bool dpsoReplace(const char* src, const char* dst);


/**
 * Synchronize file state with storage device.
 *
 * The function transfers all modified data (and possibly file
 * attributes) to the permanent storage device. It should normally be
 * preceded by fflush().
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * false.
 */
bool dpsoSyncFile(FILE* fp);


/**
 * Synchronize directory containing filePath with storage device.
 *
 * The function is an equivalent of dpsoSyncFile() for directories:
 * it's usually called after creating a file to ensure that the new
 * directory entry has reached the storage device.
 *
 * On failure, sets an error message (dpsoGetError()) and returns
 * false.
 */
bool dpsoSyncFileDir(const char* filePath);


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
