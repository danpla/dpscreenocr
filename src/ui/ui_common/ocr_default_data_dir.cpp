#include "ocr_default_data_dir.h"

#include "dpso_ext/dpso_ext.h"
#include "dpso_utils/error_get.h"
#include "dpso_utils/error_set.h"
#include "dpso_utils/os.h"

#include "file_names.h"


namespace ui {


std::optional<std::string> getDefaultOcrDataDir(
    const DpsoOcrEngineInfo& engineInfo)
{
    std::string result;

    switch (engineInfo.dataDirPreference) {
    case DpsoOcrEngineDataDirPreferenceNoDataDir:
    case DpsoOcrEngineDataDirPreferencePreferDefault:
        break;
    case DpsoOcrEngineDataDirPreferencePreferExplicit: {
        const auto* dataPath = dpsoGetUserDir(
            DpsoUserDirData, uiAppFileName);
        if (!dataPath) {
            dpso::setError(
                "Can't get user data dir: {}", dpsoGetError());
            return {};
        }

        result = dataPath;
        result += *dpso::os::dirSeparators;
        result += engineInfo.id;
        result += "_data";
        break;
    }
    }

    return result;
}


}
