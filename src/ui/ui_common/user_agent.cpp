#include "user_agent.h"

#include <string_view>

#include "app_info.h"


namespace ui {
namespace {


// See RFC 1945.
bool isValidTokenChar(char c)
{
    static const std::string_view specials{"()<>@,;:\\\"/[]?={} \t"};
    return c > 31 && c < 127 && specials.find(c) == specials.npos;
}


void appendAsToken(std::string& result, std::string_view str)
{
    for (auto c : str)
        if (isValidTokenChar(c))
            result += c;
}


}


std::string getUserAgent()
{
    std::string result;

    appendAsToken(result, uiAppName);
    result += '/';
    appendAsToken(result, uiAppVersion);

    return result;
}


}
