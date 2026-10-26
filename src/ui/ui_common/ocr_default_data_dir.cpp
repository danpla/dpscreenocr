#include "ocr_default_data_dir.h"

#include "dpso_ext/dpso_ext.h"
#include "dpso_utils/error_get.h"
#include "dpso_utils/error_set.h"
#include "dpso_utils/os.h"

#include "file_names.h"


namespace ui {


std::string getDefaultOcrDataDir(std::string_view ocrEngineId)
{
    const auto* dataDirPath = dpsoGetUserDir(DpsoUserDirData);
    if (!dataDirPath) {
        dpso::setError("Can't get user data dir: {}", dpsoGetError());
        return {};
    }

    return dpso::os::joinPath({
        dataDirPath,
        uiAppFileName,
        std::string{ocrEngineId} + "_data"});
}


}
