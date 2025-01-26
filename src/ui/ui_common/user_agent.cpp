#include "user_agent.h"

#include <cstring>
#include <string>

#include "app_info.h"


namespace {


// See RFC 1945.
bool isValidTokenChar(char c)
{
    return
        c > 31
        && c < 127
        && !std::strchr("()<>@,;:\\\"/[]?={} \t", c);
}


void appendAsToken(std::string& result, const char* str)
{
    for (const char* s = str; *s; ++s)
        if (isValidTokenChar(*s))
            result += *s;
}


std::string buildUserAgent()
{
    std::string result;

    appendAsToken(result, uiAppName);
    result += '/';
    appendAsToken(result, uiAppVersion);

    return result;
}


}


const char* uiGetUserAgent(void)
{
    static const auto userAgent = buildUserAgent();
    return userAgent.c_str();
}
