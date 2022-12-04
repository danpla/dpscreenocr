
#pragma once

#include <memory>

#include "ocr_engine/ocr_engine.h"


namespace dpso {
namespace ocr {


std::unique_ptr<OcrEngine> createTesseractOcrEngine(
    const OcrEngineArgs& args);


}
}
