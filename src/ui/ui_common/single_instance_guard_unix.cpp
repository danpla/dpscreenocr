
#include "single_instance_guard.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>

#include "dpso/error.h"


struct UiSingleInstanceGuard {
    std::string filePath;
    int fd;
};


UiSingleInstanceGuard* uiSingleInstanceGuardCreate(const char* id)
{
    const auto filePath =
        std::string{"/tmp/."} + id + "_instance_lock";

    const auto fd = open(
        filePath.c_str(),
        O_WRONLY | O_CREAT,
        S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
    if (fd == -1) {
        dpsoSetError(
            "open(\"%s\", ...): %s",
            filePath.c_str(), strerror(errno));
        return {};
    }

    if (lockf(fd, F_TLOCK, 0) == 0)
        return new UiSingleInstanceGuard{filePath, fd};

    close(fd);

    if (errno == EACCES || errno == EAGAIN)
        return new UiSingleInstanceGuard{{}, -1};

    dpsoSetError("lockf(): %s", strerror(errno));
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
