
#include "libintl_noop.h"


#if !ENABLE_NLS


const char* gettext(const char* msgId)
{
    return msgId;
}


const char* ngettext(
    const char* msgId1, const char* msgId2, unsigned long n)
{
    return n == 1 ? msgId1 : msgId2;
}


const char* textdomain(const char* domainName)
{
    return domainName;
}


const char* bind_textdomain_codeset(
    const char* domainName, const char* codeset)
{
    (void)domainName;
    return codeset;
}


#endif  // !ENABLE_NLS
