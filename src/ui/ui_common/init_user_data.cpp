#include "init_user_data.h"

#include <filesystem>
#include <system_error>

#include "dpso_utils/error_get.h"
#include "dpso_utils/error_set.h"
#include "dpso_utils/str.h"

#include "app_dirs.h"
#include "ocr_default_data_dir.h"


namespace ui {
namespace {


namespace fs = std::filesystem;


void setupEntry(
    const fs::path& srcEntryDirPath, const fs::path& dstEntryPath)
{
    if (fs::exists(dstEntryPath))
        return;

    const auto dstEntryName = dstEntryPath.filename();
    if (dstEntryName.empty())
        return;

    const auto srcEntryPath = srcEntryDirPath / dstEntryName;
    if (!fs::exists(srcEntryPath))
        return;

    const auto dstEntryDirPath = dstEntryPath.parent_path();
    if (dstEntryDirPath.empty())
        return;

    fs::create_directories(dstEntryDirPath);
    fs::copy(
        srcEntryPath,
        dstEntryPath,
        fs::copy_options::skip_existing
            | fs::copy_options::recursive);
}


bool setupOcrData()
{
    const auto appDataDir = fs::u8path(uiGetAppDir(UiAppDirData));

    for (int i = 0; i < dpsoOcrGetNumEngines(); ++i) {
        DpsoOcrEngineInfo ocrEngineInfo;
        dpsoOcrGetEngineInfo(i, &ocrEngineInfo);

        const auto dataDir = getDefaultOcrDataDir(ocrEngineInfo);
        if (!dataDir) {
            dpso::setError(
                "Can't get default data dir for {} OCR engine: {}",
                ocrEngineInfo.name, dpsoGetError());
            return false;
        }

        if (dataDir->empty())
            continue;

        try {
            setupEntry(appDataDir, fs::u8path(*dataDir));
        } catch (fs::filesystem_error& e) {
            dpso::setError(
                "Can't set up data for {} OCR engine ({}): {}",
                ocrEngineInfo.name, *dataDir, e.what());
            return false;
        }
    }

    return true;
}


}


bool initUserData()
{
    if (!setupOcrData()) {
        dpso::setError("setupOcrData(): {}", dpsoGetError());
        return false;
    }

    return true;
}


}
