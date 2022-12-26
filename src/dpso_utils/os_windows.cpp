
#include "os.h"

#include <cerrno>
#include <cstring>
#include <io.h>

#include "dpso/error.h"
#include "windows_utils.h"


const char* const dpsoDirSeparators = "\\/";


FILE* dpsoFopen(const char* filePath, const char* mode)
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


int dpsoRemove(const char* filePath)
{
    std::wstring filePathUtf16;

    try {
        filePathUtf16 = dpso::windows::utf8ToUtf16(filePath);
    } catch (std::runtime_error&) {
        errno = EINVAL;
        return -1;
    }

    return _wremove(filePathUtf16.c_str());
}


bool dpsoSyncFile(FILE* fp)
{
    const auto fd = _fileno(fp);
    if (fd == -1) {
        dpsoSetError("_fileno(): %s", std::strerror(errno));
        return false;
    }

    if (_commit(fd) == -1) {
        dpsoSetError("_commit(): %s", std::strerror(errno));
        return false;
    }

    return true;
}


bool dpsoSyncFileDir(const char* filePath)
{
    (void)filePath;
    // Windows doesn't support directory synchronization.
    return true;
}
