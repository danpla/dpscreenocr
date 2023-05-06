
#include "os.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "error.h"
#include "unix/fsync.h"


const char* const dpsoDirSeparators = "/";


int64_t dpsoGetFileSize(const char* filePath)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        dpsoSetError("stat(): %s", strerror(errno));
        return -1;
    }

    return st.st_size;
}


FILE* dpsoFopen(const char* filePath, const char* mode)
{
    return fopen(filePath, mode);
}


int dpsoRemove(const char* filePath)
{
    return remove(filePath);
}


bool dpsoReplace(const char* src, const char* dst)
{
    if (rename(src, dst) == -1) {
        dpsoSetError("rename(): %s", strerror(errno));
        return false;
    }

    return true;
}


bool dpsoSyncFile(FILE* fp)
{
    const auto fd = fileno(fp);
    if (fd == -1) {
        dpsoSetError("fileno(): %s", strerror(errno));
        return false;
    }

    if (dpso::unix::fsync(fd) == -1) {
        dpsoSetError(
            "unix::fsync(): %s", strerror(errno));
        return false;
    }

    return true;
}


bool dpsoSyncFileDir(const char* filePath)
{
    std::string dirPath;
    if (const auto* sep = strrchr(filePath, '/'))
        // Include the separator in case the file is in the root
        // directory.
        dirPath.assign(filePath, sep - filePath + 1);
    else
        dirPath = ".";

    #ifndef O_DIRECTORY
    #define O_DIRECTORY 0
    #endif

    const auto fd = open(dirPath.c_str(), O_RDONLY | O_DIRECTORY);
    if (fd == -1) {
        if (errno == EACCES)
            return true;

        dpsoSetError(
            "open() for directory failed: %s", strerror(errno));
        return false;
    }

    // Some systems can't fsync() a directory, so ignore errors.
    dpso::unix::fsync(fd);
    close(fd);

    return true;
}
