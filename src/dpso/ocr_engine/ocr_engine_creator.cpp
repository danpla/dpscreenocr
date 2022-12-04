
#include "ocr_engine/ocr_engine_creator.h"

#include "ocr_engine/tesseract/tesseract_ocr_engine_creator.h"


namespace dpso {
namespace ocr {


static const OcrEngineCreator* const creators[] = {
    &getTesseractOcrEngineCreator()
};


const OcrEngineCreator& OcrEngineCreator::get(std::size_t idx)
{
    return *creators[idx];
}


std::size_t OcrEngineCreator::getCount()
{
    return sizeof(creators) / sizeof(*creators);
}


}
}
