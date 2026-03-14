#pragma once

#include <memory>
#include <string_view>

#include "engine/recognizer.h"


namespace dpso::ocr::tesseract {


std::unique_ptr<Recognizer> createRecognizer(
    std::string_view dataDir);


}
