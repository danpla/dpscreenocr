
#include "ocr_engine/ocr_engine.h"

#include "ocr_engine/tesseract/tesseract_ocr_engine.h"


namespace dpso {


std::unique_ptr<OcrEngine> OcrEngine::create()
{
    return createTesseractOcrEngine();
}


}
