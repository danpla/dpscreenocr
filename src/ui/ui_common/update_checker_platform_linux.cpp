
#include "update_checker_platform.h"

#include <gnu/libc-version.h>

#include "dpso_utils/version_cmp.h"

#include "update_checker.h"


const char* uiUpdateCheckerGetPlatformId(void)
{
    return "linux";
}


namespace ui::updateChecker {


std::vector<UnmetRequirement> processRequirements(
    const dpso::json::Object& requirements)
{
    const auto requiredVersion = requirements.getStr("glibc");
    const auto* actualVersion = gnu_get_libc_version();

    if (dpso::VersionCmp{actualVersion}
            < dpso::VersionCmp{requiredVersion.c_str()}) {
        return {
            {
                "glibc " + requiredVersion,
                "glibc " + std::string{actualVersion}}};
    }

    return {};
}


}

