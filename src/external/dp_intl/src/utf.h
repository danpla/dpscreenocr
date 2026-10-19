#pragma once

#include <stdexcept>
#include <string_view>


namespace dp::intl {


// Throws intl::Error on invalid UTF-8.
void validateUtf8(std::string_view s);


}
