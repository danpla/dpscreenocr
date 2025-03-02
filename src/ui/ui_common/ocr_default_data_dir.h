#pragma once

#include <optional>
#include <string>

#include "dpso_ocr/engine.h"


namespace ui {


// On failure, sets an error message (dpsoGetError()) and returns
// nullopt.
std::optional<std::string> getDefaultOcrDataDir(
    const DpsoOcrEngineInfo& engineInfo);


}
