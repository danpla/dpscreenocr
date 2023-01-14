
#include "unix/fsync.h"

#ifdef __APPLE__
#include <fcntl.h>
#endif
#include <unistd.h>


namespace dpso::unix {


int fsync(int fd)
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


}
