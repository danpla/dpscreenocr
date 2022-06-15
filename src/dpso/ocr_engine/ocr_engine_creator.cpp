
#include "ocr_engine/ocr_engine_creator.h"

#include <cstring>
#include <utility>

#include "ocr_engine/tesseract/tesseract_ocr_engine_creator.h"


namespace dpso {


static std::vector<std::unique_ptr<OcrEngineCreator>> customCreators;
static std::vector<const OcrEngineCreator*> allCreators{
    &getTesseractOcrEngineCreator()
};


const std::vector<const OcrEngineCreator*>&
OcrEngineCreator::getAll()
{
    return allCreators;
}


void OcrEngineCreator::add(
    std::unique_ptr<OcrEngineCreator> creator)
{
    if (!creator)
        return;

    customCreators.push_back(std::move(creator));
    allCreators.push_back(customCreators.back().get());
}


const OcrEngineCreator* OcrEngineCreator::find(const char* id)
{
    for (const auto* creator : allCreators)
        if (std::strcmp(id, creator->getInfo().id) == 0)
            return creator;

    return nullptr;
}


}
