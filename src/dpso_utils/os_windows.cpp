
#include "os.h"

#include <cerrno>

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
