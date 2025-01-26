#include "ocr_data_utils.h"

#include <cassert>
#include <string>

#include "app_info.h"


const char* uiGetOcrDataInfoFileUrl(
    const DpsoOcrEngineInfo* ocrEngineInfo)
{
    if (!ocrEngineInfo)
        return "";

    static std::string result;

    result = uiAppWebsite;
    assert(!result.empty());

    if (result.back() != '/')
        result += '/';

    result += "ocr_engine_data/";
    result += ocrEngineInfo->id;
    result += "_data.json";

    return result.c_str();
}


const char* uiGetOcrDataDirName(
    const DpsoOcrEngineInfo* ocrEngineInfo)
{
    if (!ocrEngineInfo)
        return "";

    static std::string result;
    result.clear();

    switch (ocrEngineInfo->dataDirPreference) {
    case DpsoOcrEngineDataDirPreferenceNoDataDir:
    case DpsoOcrEngineDataDirPreferencePreferDefault:
        break;
    case DpsoOcrEngineDataDirPreferencePreferExplicit:
        result = ocrEngineInfo->id;
        result += "_data";
        break;
    }

    return result.c_str();
}
