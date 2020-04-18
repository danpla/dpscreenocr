
#include "os.h"

#include <cerrno>

#include "windows_utils.h"


namespace dpso {


const char dirSeparator = '\\';


std::FILE* fopenUtf8(const char* fileName, const char* mode)
{
    std::wstring fileNameUtf16;
    std::wstring modeUtf16;

    try {
        fileNameUtf16 = win::utf8ToUtf16(fileName);
        modeUtf16 = win::utf8ToUtf16(mode);
    } catch (std::runtime_error&) {
        errno = EINVAL;
        return nullptr;
    }

    return _wfopen(fileNameUtf16.c_str(), modeUtf16.c_str());
}


}
