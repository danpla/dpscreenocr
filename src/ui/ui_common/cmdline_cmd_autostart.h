
#pragma once


namespace ui {


// On failure, sets an error message (dpsoGetError()) and returns
// false.
bool cmdLineCmdAutostart(const char* argv0, const char* action);


}
