// This file contains platform-specific parts of the update checker.
// The corresponding CPP is also a good place for the definition of
// uiUpdateCheckerGetPlatformId().

#pragma once

#include <stdexcept>
#include <string>
#include <vector>

#include "dpso_json/json.h"


namespace ui::updateChecker {


class RequirementsError : public std::runtime_error {
    using runtime_error::runtime_error;
};


// See UiUpdateCheckerUnmetRequirement.
struct UnmetRequirement {
    std::string required;
    std::string actual;
};


// Process platform-specific minimum requirements. Returns a list of
// unmet requirements, or an empty list if all requirements are met.
// Throws RequirementsError on any errors during checking. For
// convenience, can also throw dpso::json::Error on JSON-specific
// errors.
std::vector<UnmetRequirement> processRequirements(
    const dpso::json::Object& requirements);


}
