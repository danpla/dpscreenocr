#include "update_checker_utils.h"

#include <cassert>
#include <string>

#include "app_info.h"
#include "update_checker.h"


const char* uiUpdateCheckerGetInfoFileUrl(void)
{
    if (!uiUpdateCheckerIsAvailable())
        return "";

    static std::string result;

    result = uiAppWebsite;
    assert(!result.empty());

    if (result.back() != '/')
        result += '/';

    result += "version_info/";
    result += uiUpdateCheckerGetPlatformId();
    result += ".json";

    return result.c_str();
}
