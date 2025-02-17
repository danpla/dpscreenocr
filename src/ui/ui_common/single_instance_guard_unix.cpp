#include "single_instance_guard.h"

#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>

#include "dpso_utils/error_set.h"
#include "dpso_utils/os.h"


struct UiSingleInstanceGuard {
    std::string filePath;
    int fd;
};


UiSingleInstanceGuard* uiSingleInstanceGuardCreate(const char* id)
{
    // Since the main goal of the guard is to protect access to data
    // in $XDG_*_HOME directories, the lock file should be associated
    // with the current user. When placing the file in /tmp (which can
    // be shared among users), we do this by putting the UID or
    // username in the file name.
    //
    // There are other possible places for the lock file (note that
    // they are already associated with the current user and don't
    // need a unique name for the lock file):
    //
    // * $XDG_RUNTIME_DIR, which in practice is set to /run/user/(PID)
    //
    // * $XDG_CACHE_HOME or another dir inside $HOME

    const auto uid = getuid();

    // Use the name instead of UID for aesthetic purposes.
    errno = 0;
    const auto* passwd = getpwuid(uid);
    if (!passwd) {
        dpso::setError(
            "getpwuid({}): {}",
            uid,
            errno == 0
                ? "Can't find the user"
                : dpso::os::getErrnoMsg(errno));
        return {};
    }

    const auto filePath =
        std::string{"/tmp/."}
        + id
        + "_instance_lock_for_"
        + passwd->pw_name;

    const auto fd = open(
        filePath.c_str(), O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR);
    if (fd == -1) {
        dpso::setError(
            "open(\"{}\", ...): {}",
            filePath, dpso::os::getErrnoMsg(errno));
        return {};
    }

    if (lockf(fd, F_TLOCK, 0) == 0)
        return new UiSingleInstanceGuard{filePath, fd};

    close(fd);

    if (errno == EACCES || errno == EAGAIN)
        return new UiSingleInstanceGuard{{}, -1};

    dpso::setError("lockf(): {}", dpso::os::getErrnoMsg(errno));
    return {};
}


void uiSingleInstanceGuardDelete(UiSingleInstanceGuard* guard)
{
    if (!guard)
        return;

    if (guard->fd != -1) {
        close(guard->fd);
        unlink(guard->filePath.c_str());
    }

    delete guard;
}


bool uiSingleInstanceGuardIsPrimary(
    const UiSingleInstanceGuard* guard)
{
    return guard && guard->fd != -1;
}
