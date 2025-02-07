#include "update_checker_default.h"

#include <cassert>
#include <string>

#include "app_info.h"
#include "user_agent.h"


static std::string getInfoFileUrl()
{
    if (!uiUpdateCheckerIsAvailable())
        return {};

    std::string result = uiAppWebsite;
    assert(!result.empty());

    if (result.back() != '/')
        result += '/';

    result += "version_info/";
    result += uiUpdateCheckerGetPlatformId();
    result += ".json";

    return result;
}


UiUpdateChecker* uiUpdateCheckerCreateDefault(void)
{
    return uiUpdateCheckerCreate(
        uiAppVersion, uiGetUserAgent(), getInfoFileUrl().c_str());
}
