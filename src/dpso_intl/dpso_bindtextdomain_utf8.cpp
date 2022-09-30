
#include "dpso_bindtextdomain_utf8.h"


#if ENABLE_NLS

#include <libintl.h>


#ifdef _WIN32

#include <string>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


static std::wstring utf8ToUtf16(const char* utf8Str)
{
    const auto sizeWithNull = MultiByteToWideChar(
        CP_UTF8, 0, utf8Str,
        // Tell that the string is null-terminated; the returned size
        // will also include the null.
        -1,
        nullptr, 0);
    if (sizeWithNull <= 0)
        return {};

    // The C++ standard allows to overwrite string[size()] with 0.
    std::wstring result(sizeWithNull - 1, 0);

    if (!MultiByteToWideChar(
            CP_UTF8, 0,
            utf8Str, -1,
            &result[0], sizeWithNull))
        return {};

    return result;
}


void bindtextdomainUtf8(const char* domainName, const char* dirName)
{
    if (!dirName)
        return;

    wbindtextdomain(domainName, utf8ToUtf16(dirName).c_str());
}



#else  // _WIN32


void bindtextdomainUtf8(const char* domainName, const char* dirName)
{
    bindtextdomain(domainName, dirName);
}


#endif // !_WIN32


#else  // ENABLE_NLS


void bindtextdomainUtf8(const char* domainName, const char* dirName)
{
    (void)domainName;
    (void)dirName;
}


#endif  // !ENABLE_NLS