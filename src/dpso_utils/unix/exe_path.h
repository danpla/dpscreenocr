#pragma once

#include <string>
#include <string_view>


namespace dpso::unix {


// Return a canonical absolute path of the given executable. The name
// can be either a basename (to be searched in $PATH) or a path
// (absolute or relative). Throws os::Error.
std::string getExePath(std::string_view name);


}
