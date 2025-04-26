#pragma once

#include <memory>

#include "engine/recognizer.h"


namespace dpso::ocr::tesseract {


std::unique_ptr<Recognizer> createRecognizer(const char* dataDir);


}
