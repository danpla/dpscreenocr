#include "engine/tesseract/lang_manager.h"

#include "engine/lang_manager_error.h"
#include "engine/remote_files_lang_manager.h"
#include "engine/tesseract/lang_names.h"
#include "engine/tesseract/lang_utils.h"


namespace dpso::ocr::tesseract {
namespace {


std::vector<std::string> getLocalLangs(const char* dataDir)
{
    try {
        return getAvailableLangs(dataDir);
    } catch (Error& e) {
        throw LangManagerError{
            std::string{"Can't get available languages: "}
            + e.what()};
    }
}


class LangManager : public RemoteFilesLangManager {
public:
    LangManager(
            const char* dataDir,
            const char* userAgent,
            const char* infoFileUrl)
        : RemoteFilesLangManager{
            dataDir,
            traineddataExt,
            userAgent,
            infoFileUrl,
            getLocalLangs(dataDir)}
    {
    }

    bool shouldIgnoreLang(const char* langCode) const override
    {
        return isIgnoredLang(langCode);
    }

    std::string getLangName(const char* langCode) const override
    {
        const auto* name = tesseract::getLangName(langCode);
        return name ? name : "";
    }
};


}


bool hasLangManager()
{
    return true;
}


std::unique_ptr<ocr::LangManager> createLangManager(
    const char* dataDir,
    const char* userAgent,
    const char* infoFileUrl)
{
    return std::make_unique<LangManager>(
        dataDir, userAgent, infoFileUrl);
}


}
