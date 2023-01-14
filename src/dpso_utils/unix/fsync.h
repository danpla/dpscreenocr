
#pragma once


namespace dpso::unix {


/**
 * fsync() wrapper with F_FULLFSYNC support on macOS.
 */
int fsync(int fd);


}
