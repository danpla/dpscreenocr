#pragma once

#include <string>
#include <vector>


namespace ui {


// Return the list of preferred UI languages from the system settings.
//
// Each language in the list has the format similar to BCP 47, and
// consist of one or more alphanumeric subtags separated either by
// hyphens or underscores. See the documentation of the dp_intl
// library for the detailed description of the tag format.
std::vector<std::string> getPreferredTranslationLangs();


}
