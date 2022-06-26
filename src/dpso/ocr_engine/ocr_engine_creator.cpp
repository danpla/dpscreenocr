
#include "ocr_engine/ocr_engine_creator.h"

#include <cstring>
#include <utility>
#include <vector>

#include "ocr_engine/tesseract/tesseract_ocr_engine_creator.h"


namespace dpso {


static std::vector<const OcrEngineCreator*> allCreators{
    &getTesseractOcrEngineCreator()
};
static std::vector<std::unique_ptr<OcrEngineCreator>> customCreators;


const OcrEngineCreator& OcrEngineCreator::get(std::size_t idx)
{
    return *allCreators[idx];
}


std::size_t OcrEngineCreator::getCount()
{
    return allCreators.size();
}


void OcrEngineCreator::add(
    std::unique_ptr<OcrEngineCreator> creator)
{
    if (!creator)
        return;

    allCreators.push_back(creator.get());
    customCreators.push_back(std::move(creator));
}


const OcrEngineCreator* OcrEngineCreator::find(const char* id)
{
    for (const auto* creator : allCreators)
        if (std::strcmp(id, creator->getInfo().id) == 0)
            return creator;

    return nullptr;
}


}
