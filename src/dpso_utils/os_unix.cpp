#include "os.h"

#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <vector>


namespace dpso::os {
namespace {


[[noreturn]]
void throwErrno(const char* description)
{
    os::throwErrno(description, errno);
}


struct PathParts {
    const char* dirBegin;
    const char* dirEnd;
    const char* baseBegin;
    const char* baseEnd;
};


PathParts splitPath(const char* path)
{
    const auto* sepBegin = path;
    const auto* sepEnd = sepBegin;

    const auto* s = path;

    for (; *s; ++s)
        if (*s == '/') {
            if (s > path && s[-1] != '/')
                sepBegin = s;

            sepEnd = s + 1;
        }

    return {path, sepBegin == path ? sepEnd : sepBegin, sepEnd, s};
}


struct LocaleFreer {
    using pointer = locale_t;

    void operator()(locale_t locobj) const
    {
        if (locobj)
            freelocale(locobj);
    }
};


using LocaleUPtr = std::unique_ptr<locale_t, LocaleFreer>;


LocaleUPtr newLocale(
    int categoryMask, const char* locale, locale_t base)
{
    return LocaleUPtr{newlocale(categoryMask, locale, base)};
}


}


std::string getErrnoMsg(int errnum)
{
    static const auto cLocale = newLocale(LC_ALL_MASK, "C", nullptr);
    return cLocale
        ? strerror_l(errnum, cLocale.get()) : strerror(errnum);
}


const char* const newline = "\n";
const char* const dirSeparators = "/";


std::string getDirName(const char* path)
{
    const auto parts = splitPath(path);
    return {parts.dirBegin, parts.dirEnd};
}


std::string getBaseName(const char* path)
{
    const auto parts = splitPath(path);
    return {parts.baseBegin, parts.baseEnd};
}


std::string convertUtf8PathToSys(const char* utf8Path)
{
    return utf8Path;
}


std::int64_t getFileSize(const char* filePath)
{
    struct stat st;
    if (stat(filePath, &st) != 0)
        throwErrno("stat()");

    return st.st_size;
}


void resizeFile(const char* filePath, std::int64_t newSize)
{
    if (truncate(filePath, newSize) != 0)
        throwErrno("truncate()");
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
        const auto ret = mkdir(dirPath, c ? 0777 : mode);
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

    if (os::fsync(fd) == -1)
        throwErrno("os::fsync()");
}


void syncDir(const char* dirPath)
{
    #ifndef O_DIRECTORY
    #define O_DIRECTORY 0
    #endif

    const auto fd = open(dirPath, O_RDONLY | O_DIRECTORY);
    if (fd == -1) {
        if (errno == EACCES)
            return;

        throwErrno("open() directory");
    }

    // Some systems can't fsync() a directory, so ignore errors.
    os::fsync(fd);
    close(fd);
}


void exec(
    const char* exePath,
    const char* const args[],
    std::size_t numArgs)
{
    const auto pid = fork();
    if (pid == -1)
        throwErrno("fork()");

    if (pid == 0) {
        std::vector<const char*> argv;
        argv.reserve(1 + numArgs + 1);

        argv.push_back(exePath);
        argv.insert(argv.end(), args, args + numArgs);
        argv.push_back(nullptr);

        execvp(argv[0], (char* const*)argv.data());
        _Exit(EXIT_FAILURE);
    }

    if (waitpid(pid, nullptr, 0) == -1)
        throwErrno("waitpid()");
}


}
