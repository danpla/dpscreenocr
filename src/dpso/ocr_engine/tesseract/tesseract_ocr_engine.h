
#pragma once

#include "ocr_engine/ocr_engine.h"


namespace dpso {


std::unique_ptr<OcrEngine> createTesseractOcrEngine(
    const OcrEngineArgs& args);


}
