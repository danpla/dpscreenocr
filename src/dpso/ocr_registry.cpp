
#include "ocr_registry.h"

#include <cassert>


namespace dpso::ocr {


void onLangManagerCreated(DpsoOcr& ocr);
void onLangManagerDeleted(DpsoOcr& ocr);


static std::vector<std::weak_ptr<OcrRegistry>> cache;


std::shared_ptr<OcrRegistry> OcrRegistry::get(
    const char* engineId, const char* dataDir)
{
    for (const auto& regWPtr : cache) {
        auto reg = regWPtr.lock();

        // A custom deleter will remove OcrRegistry from the cache.
        assert(reg);

        if (reg
                && reg->engineId == engineId
                && reg->dataDir == dataDir)
            return reg;
    }

    std::shared_ptr<OcrRegistry> reg{
        new OcrRegistry{engineId, dataDir},
        [](OcrRegistry* reg)
        {
            for (auto iter = cache.begin();
                    iter < cache.end();
                    ++iter)
                if (iter->lock().get() == reg) {
                    cache.erase(iter);
                    break;
                }

            delete reg;
        }};

    cache.push_back(reg);

    return reg;
}


OcrRegistry::OcrRegistry(const char* engineId, const char* dataDir)
    : engineId{engineId}
    , dataDir{dataDir}
    , ocrs{}
    , langManagerIsActive{}
{
}


void OcrRegistry::add(DpsoOcr& ocr)
{
    for (auto* existingOcr : ocrs)
        if (&ocr == existingOcr)
            return;

    ocrs.push_back(&ocr);
}


void OcrRegistry::remove(DpsoOcr& ocr)
{
    for (auto iter = ocrs.begin(); iter < ocrs.end(); ++iter)
        if (*iter == &ocr) {
            ocrs.erase(iter);
            break;
        }
}


void OcrRegistry::langManagerCreated()
{
    langManagerIsActive = true;

    for (auto* ocr : ocrs)
        onLangManagerCreated(*ocr);
}


void OcrRegistry::langManagerDeleted()
{
    langManagerIsActive = false;

    for (auto* ocr : ocrs)
        onLangManagerDeleted(*ocr);
}


bool OcrRegistry::getLangManagerIsActive() const
{
    return langManagerIsActive;
}


}
