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
        const auto* dataDirPath = dpsoGetUserDir(DpsoUserDirData);
        if (!dataDirPath) {
            dpso::setError(
                "Can't get user data dir: {}", dpsoGetError());
            return {};
        }

        result = dpso::os::joinPath({
            dataDirPath,
            uiAppFileName,
            std::string{engineInfo.id} + "_data"});
        break;
    }
    }

    return result;
}


}
