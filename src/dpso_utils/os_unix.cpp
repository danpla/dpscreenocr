
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
#include "unix/make_dirs.h"


namespace dpso::os {


const char* const dirSeparators = "/";


[[noreturn]]
static void throwErrno(const char* description)
{
    const auto message =
        std::string{description} + ": " + strerror(errno);

    switch (errno) {
    case ENOENT:
        throw FileNotFoundError{message};
    default:
        throw Error{message};
    }
}


std::int64_t getFileSize(const char* filePath)
{
    struct stat st;
    if (stat(filePath, &st) != 0)
        throwErrno("stat()");

    return st.st_size;
}


std::FILE* fopen(const char* filePath, const char* mode)
{
    return ::fopen(filePath, mode);
}


void removeFile(const char* filePath)
{
    if (unlink(filePath) != 0)
        throwErrno("unlink()");
}


void replace(const char* src, const char* dst)
{
    if (rename(src, dst) != 0)
        throwErrno("rename()")
}


void makeDirs(const char* dirPath)
{
    if (!unix::makeDirs(dirPath))
        throwErrno("unix::makeDirs()");
}


void syncFile(std::FILE* fp)
{
    const auto fd = fileno(fp);
    if (fd == -1) {
        throwErrno("fileno()");

    if (unix::fsync(fd) == -1)
        throwErrno("unix::fsync()");
}


void syncFileDir(const char* filePath)
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
        if (errno != EACCES)
            throwErrno("open() directory");

        return;
    }

    // Some systems can't fsync() a directory, so ignore errors.
    unix::fsync(fd);
    close(fd);
}


}
