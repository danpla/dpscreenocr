#pragma once

#include <optional>
#include <string>

#include "dpso_ocr/engine.h"


namespace ui {


// Return an absolute path of the OCR data directory for the given
// engine. The path will be empty if
// DpsoOcrEngineInfo::dataDirPreference is not
// DpsoOcrEngineDataDirPreferencePreferExplicit.
//
// On failure, sets an error message (dpsoGetError()) and returns
// nullopt.
std::optional<std::string> getDefaultOcrDataDir(
    const DpsoOcrEngineInfo& engineInfo);


}
