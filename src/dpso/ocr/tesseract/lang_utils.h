
#pragma once

#include <string>
#include <vector>


namespace dpso::ocr::tesseract {


extern const char* const traineddataExt;


// Returns true if the language should be ignored, e.g., when it's not
// a real language but an auxiliary data, such as "equ" or "osd".
bool isIgnoredLang(const char* lang);


// Returns languages from TessBaseAPI::GetAvailableLanguagesAsVector,
// excluding those that are ignored by isIgnoredLang().
//
// Throws ocr::Error.
std::vector<std::string> getAvailableLangs(const char* dataDir);


}
