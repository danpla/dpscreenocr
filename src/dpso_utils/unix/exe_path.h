
#pragma once

#include <string>


namespace dpso::unix {


// Return a canonical absolute path of the given executable. The name
// can be either a basename (to be searched in $PATH) or a path
// (absolute or relative). Throws os::Error on errors.
std::string getExePath(const char* name);


}
