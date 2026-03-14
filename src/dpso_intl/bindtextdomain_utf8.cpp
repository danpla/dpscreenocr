#include "bindtextdomain_utf8.h"

#include <libintl.h>


#ifdef _WIN32

#include "dpso_utils/windows/utf.h"


void bindtextdomainUtf8(const char* domainName, const char* dirName)
{
    // dirName can be null to return the previously set directory. Our
    // UTF-8 wrapper returns nothing, so skip this case.
    if (!dirName)
        return;

    try {
        wbindtextdomain(
            domainName, dpso::windows::utf8ToUtf16(dirName).c_str());
    } catch (dpso::windows::CharConversionError&) {
    }
}


#else  // _WIN32


void bindtextdomainUtf8(const char* domainName, const char* dirName)
{
    bindtextdomain(domainName, dirName);
}


#endif // !_WIN32
