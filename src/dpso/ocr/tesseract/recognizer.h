
#pragma once

#include <memory>

#include "ocr/recognizer.h"


namespace dpso::ocr::tesseract {


std::unique_ptr<Recognizer> createRecognizer(const char* data);


}
