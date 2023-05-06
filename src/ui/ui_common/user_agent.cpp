
#include "user_agent.h"

#include <cstring>
#include <string>

#include "app_info.h"


// See RFC 1945.
static bool isValidTokenChar(char c)
{
    return
        c > 0x0F
        && c != 0x7F
        && !std::strchr("()<>@,;:\\\"/[]?={} \t", c);
}


static void appendAsToken(std::string& result, const char* str)
{
    for (const char* s = str; *s; ++s)
        if (isValidTokenChar(*s))
            result += *s;
}


static std::string buildUserAgent()
{
    std::string result;

    appendAsToken(result, uiAppName);
    result += '/';
    appendAsToken(result, uiAppVersion);

    return result;
}


const char* uiGetUserAgent(void)
{
    static const std::string userAgent{buildUserAgent()};
    return userAgent.c_str();
}
