#include "ocr_default.h"

#include <cassert>

#include "app_info.h"
#include "ocr_default_data_dir.h"
#include "user_agent.h"


DpsoOcr* dpsoOcrCreateDefault(int engineIdx)
{
    DpsoOcrEngineInfo engineInfo;
    dpsoOcrGetEngineInfo(engineIdx, &engineInfo);

    const auto dataDir = ui::getDefaultOcrDataDir(engineInfo);
    if (!dataDir)
        return {};

    return dpsoOcrCreate(engineIdx, dataDir->c_str());
}


static std::string getInfoFileUrl(const DpsoOcrEngineInfo& engineInfo)
{
    std::string result{uiAppWebsite};
    assert(!result.empty());

    if (result.back() != '/')
        result += '/';

    result += "ocr_engine_data/";
    result += engineInfo.id;
    result += "_data.json";

    return result;
}


DpsoOcrLangManager* dpsoOcrLangManagerCreateDefault(int engineIdx)
{
    DpsoOcrEngineInfo engineInfo;
    dpsoOcrGetEngineInfo(engineIdx, &engineInfo);

    const auto dataDir = ui::getDefaultOcrDataDir(engineInfo);
    if (!dataDir)
        return {};

    return dpsoOcrLangManagerCreate(
        engineIdx,
        dataDir->c_str(),
        ui::getUserAgent().c_str(),
        getInfoFileUrl(engineInfo).c_str());
}
