#include "engine.h"

#include "engine/engine.h"


using namespace dpso;


int dpsoOcrGetNumEngines(void)
{
    return ocr::Engine::getCount();
}


void dpsoOcrGetEngineInfo(int idx, DpsoOcrEngineInfo* info)
{
    if (idx < 0
            || static_cast<std::size_t>(idx)
                >= ocr::Engine::getCount()
            || !info)
        return;

    const auto& internalInfo = ocr::Engine::get(idx).getInfo();

    *info = {
        internalInfo.id.c_str(),
        internalInfo.name.c_str(),
        internalInfo.version.c_str(),
        internalInfo.hasLangManager};
}
