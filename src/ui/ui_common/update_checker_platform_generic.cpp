#include "update_checker_platform.h"

#include "update_checker.h"


bool uiUpdateCheckerIsAvailable(void)
{
    return true;
}


const char* uiUpdateCheckerGetPlatformId(void)
{
    return "generic";
}


namespace ui::updateChecker {


std::vector<UnmetRequirement> processRequirements(
    const dpso::json::Object& /*requirements*/)
{
    return {};
}


}
