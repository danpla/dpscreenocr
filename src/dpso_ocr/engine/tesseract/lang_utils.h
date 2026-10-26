#pragma once

#include <string>
#include <string_view>
#include <vector>


namespace dpso::ocr::tesseract {


extern const std::string_view traineddataExt;


// Returns true if the language should be ignored, e.g., when it's not
// a real language but an auxiliary data, such as "equ" or "osd".
bool isIgnoredLang(std::string_view lang);



// Returns the compiled-in Tesseract data path, which is necessary on
// Unix-like systems when using the system-wide Tesseract library and
// language files. This path is typically configured by the package
// maintainers and can therefore differ between OS distributions.
const std::string& getDefaultDataDir();


// Returns languages from TessBaseAPI::GetAvailableLanguagesAsVector,
// excluding those that are ignored by isIgnoredLang().
//
// Throws ocr::Error.
std::vector<std::string> getAvailableLangs(std::string_view dataDir);


}
