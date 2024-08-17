
#pragma once

#include <string>


namespace ui {


// This function is called as a part of uiInit(). On failure, sets an
// error message (dpsoGetError()) and returns false.
bool initExePath(const char* argv0);


// Return a canonical absolute path of the toplevel executable
// launched by the user.
//
// On some platforms, an application may be packaged in a way that
// requires the use of a launcher program that sets up an environment
// for the final executable. This function is intended for cases where
// it's important to use the path of the launcher rather than the
// final executable, such as when adding the application to the
// autostart.
//
// If the application doesn't use a launcher, the function returns the
// path of the running executable, i.e. the same as getExePath().
const std::string& getToplevelExePath();


// Return a canonical absolute path of the current executable, or an
// empty string if uiInit() wasn't called.
const std::string& getExePath();


}
