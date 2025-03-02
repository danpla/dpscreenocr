#pragma once

#include <string>


namespace ui {


// Get a string for the User-Agent HTTP header. The string is built
// from uiAppName and uiAppVersion.
std::string getUserAgent();


}
