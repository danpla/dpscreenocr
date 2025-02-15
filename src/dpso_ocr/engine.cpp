#include "engine.h"

#include "engine/engine.h"


int dpsoOcrGetNumEngines(void)
{
    return dpso::ocr::Engine::getCount();
}


void dpsoOcrGetEngineInfo(int idx, DpsoOcrEngineInfo* info)
{
    if (idx < 0
            || static_cast<std::size_t>(idx)
                >= dpso::ocr::Engine::getCount()
            || !info)
        return;

    const auto& internalInfo = dpso::ocr::Engine::get(idx).getInfo();

    DpsoOcrEngineDataDirPreference dataDirPreference{};
    switch (internalInfo.dataDirPreference) {
    case dpso::ocr::EngineInfo::DataDirPreference::noDataDir:
        dataDirPreference = DpsoOcrEngineDataDirPreferenceNoDataDir;
        break;
    case dpso::ocr::EngineInfo::DataDirPreference::preferDefault:
        dataDirPreference =
            DpsoOcrEngineDataDirPreferencePreferDefault;
        break;
    case dpso::ocr::EngineInfo::DataDirPreference::preferExplicit:
        dataDirPreference =
            DpsoOcrEngineDataDirPreferencePreferExplicit;
        break;
    }

    *info = {
        internalInfo.id.c_str(),
        internalInfo.name.c_str(),
        internalInfo.version.c_str(),
        dataDirPreference,
        internalInfo.hasLangManager};
}
