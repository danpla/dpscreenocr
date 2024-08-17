
#pragma once

#include <string>


namespace dpso::unix {


// Search for the given executable in the paths from the PATH
// environment variable. Returns an empty string if the executable is
// not found, or if the executable name contains slashes.
std::string findInPathEnv(const char* name);


}
