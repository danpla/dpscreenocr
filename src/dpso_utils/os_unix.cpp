#include "os.h"

#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include <vector>

#include "os_stdio.h"


namespace dpso::os {
namespace {


[[noreturn]]
void throwErrno(std::string_view description)
{
    os::throwErrno(description, errno);
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


const std::string_view newline{"\n"};
const char dirSeparator{'/'};


std::string convertUtf8PathToSys(std::string_view utf8Path)
{
    return std::string{utf8Path};
}


std::FILE* fopen(const char* filePath, const char* mode)
{
    return ::fopen(filePath, mode);
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


void syncDir(std::string_view dirPath)
{
    #ifndef O_DIRECTORY
    #define O_DIRECTORY 0
    #endif

    const auto fd = open(
        std::string{dirPath}.c_str(), O_RDONLY | O_DIRECTORY);
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
    std::string_view exePath,
    const std::string_view* args,
    std::size_t numArgs)
{
    const auto pid = fork();
    if (pid == -1)
        throwErrno("fork()");

    if (pid == 0) {
        std::vector<std::string> argvStr;
        argvStr.reserve(1 + numArgs);

        argvStr.emplace_back(exePath);
        argvStr.insert(argvStr.end(), args, args + numArgs);

        std::vector<const char*> argv;
        argv.reserve(argvStr.size() + 1);

        for (const auto& str : argvStr)
            argv.push_back(str.c_str());
        argv.push_back(nullptr);

        execvp(argv[0], (char* const*)argv.data());
        _Exit(EXIT_FAILURE);
    }

    if (waitpid(pid, nullptr, 0) == -1)
        throwErrno("waitpid()");
}


}
