#pragma once

#include <string>
#include <string_view>


namespace ui {


// This function is called as a part of uiInit(). On failure, sets an
// error message (dpsoGetError()) and returns false.
bool initExePath(std::string_view argv0);


// Return a canonical absolute path of the toplevel executable
// launched by the user.
//
// On some platforms, an application may be packaged in a way that
// requires the use of a launcher program that sets up an environment
// for the final executable. In this case, the function returns the
// path to the launcher rather than the path of the running
// executable.
const std::string& getExePath();


}
