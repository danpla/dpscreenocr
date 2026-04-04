#pragma once

#include <string_view>


namespace ui {


// On failure, sets an error message (dpsoGetError()) and returns
// false.
bool cmdLineCmdAutostart(
    std::string_view argv0, std::string_view action);


}
