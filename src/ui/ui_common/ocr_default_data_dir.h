#pragma once

#include <string>
#include <string_view>


namespace ui {


// Return an absolute path of the OCR data directory for the given
// engine id.
//
// On failure, sets an error message (dpsoGetError()) and returns an
// empty string.
std::string getDefaultOcrDataDir(std::string_view ocrEngineId);


}
