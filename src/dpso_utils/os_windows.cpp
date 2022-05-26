
#include "os.h"

#include <cerrno>
#include <cstring>
#include <io.h>

#include "dpso/error.h"
#include "windows_utils.h"


const char* const dpsoDirSeparators = "\\/";


FILE* dpsoFopenUtf8(const char* filePath, const char* mode)
{
    std::wstring filePathUtf16;
    std::wstring modeUtf16;

    try {
        filePathUtf16 = dpso::windows::utf8ToUtf16(filePath);
        modeUtf16 = dpso::windows::utf8ToUtf16(mode);
    } catch (std::runtime_error&) {
        errno = EINVAL;
        return nullptr;
    }

    return _wfopen(filePathUtf16.c_str(), modeUtf16.c_str());
}


int dpsoSyncFile(FILE* fp)
{
    const auto fd = _fileno(fp);
    if (fd == -1) {
        dpsoSetError("_fileno() failed: %s", std::strerror(errno));
        return false;
    }

    if (_commit(fd) == -1) {
        dpsoSetError("_commit() failed: %s", std::strerror(errno));
        return false;
    }

    return true;
}


int dpsoSyncFileDir(const char* filePath)
{
    (void)filePath;
    // Windows doesn't support directory synchronization.
    return true;
}
