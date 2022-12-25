
#pragma once

#include <memory>

#include "ocr_engine/ocr_engine.h"


namespace dpso::ocr {


std::unique_ptr<OcrEngine> createTesseractOcrEngine(
    const OcrEngineArgs& args);


}
