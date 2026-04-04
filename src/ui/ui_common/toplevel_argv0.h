#pragma once

#include <string_view>


namespace ui {


// Return a value of the LAUNCHER_ARGV0 environment variable, or argv0
// if LAUNCHER_ARGV0 either not set or empty.
std::string_view getToplevelArgv0(std::string_view argv0);


}
