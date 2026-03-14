#pragma once

#include <string_view>


namespace dpso::ocr::tesseract {


// Returns an empty string the language with the given code is not
// found.
std::string_view getLangName(std::string_view langCode);


}
