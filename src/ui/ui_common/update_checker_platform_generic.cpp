
#include "update_checker_platform.h"

#include "update_checker.h"


const char* uiUpdateCheckerGetPlatformId(void)
{
    return "generic";
}


namespace ui::updateChecker {


std::vector<UnmetRequirement> processRequirements(
    const dpso::json::Object& requirements)
{
    (void)requirements;
    return {};
}


}

