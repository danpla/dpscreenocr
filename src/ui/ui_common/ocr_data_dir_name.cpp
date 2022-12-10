
#include "ocr_data_dir_name.h"

#include <string>


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
