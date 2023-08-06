
#include "os.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


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
        throwErrno("rename()");
}


static void makeDirs(char* dirPath, mode_t mode)
{
    auto* s = dirPath;

    // Root always exists.
    while (*s == '/')
        ++s;

    while (*s) {
        while (*s && *s != '/')
            ++s;

        while (*s == '/')
            ++s;

        const auto c = *s;
        *s = 0;
        // Force 0777 mode for intermediate directories so that mkdir
        // can create a directory with read or write permissions
        // removed when the same permissions are used for a newly
        // created parent directory.
        const auto ret = mkdir(dirPath, *s ? 0777 : mode);
        *s = c;

        if (ret != 0 && errno != EEXIST)
            throwErrno("mkdir()");
    }
}


void makeDirs(const char* dirPath)
{
    std::string dirPathCopy{dirPath};
    makeDirs(dirPathCopy.data(), 0777);
}


static int fsync(int fd)
{
    #ifdef __APPLE__

    // See:
    // * "man fsync" on macOS
    // * https://lists.apple.com/archives/darwin-dev/2005/Feb/msg00072.html
    if (fcntl(fd, F_FULLFSYNC) != -1)
        return 0;
    // F_FULLFSYNC failure indicates that it's not supported for the
    // current file system (see "man fcntl" for the list of supported
    // file systems). Fall back to fsync().

    #endif

    return ::fsync(fd);
}


void syncFile(std::FILE* fp)
{
    const auto fd = fileno(fp);
    if (fd == -1)
        throwErrno("fileno()");

    if (fsync(fd) == -1)
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
        if (errno == EACCES)
            return;

        throwErrno("open() directory");
    }

    // Some systems can't fsync() a directory, so ignore errors.
    fsync(fd);
    close(fd);
}


}
