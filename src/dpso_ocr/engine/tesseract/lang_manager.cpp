#include "engine/tesseract/lang_manager.h"

#include "engine/lang_manager_error.h"
#include "engine/remote_files_lang_manager.h"
#include "engine/tesseract/lang_names.h"
#include "engine/tesseract/lang_utils.h"


namespace dpso::ocr::tesseract {
namespace {


std::vector<std::string> getLocalLangs(std::string_view dataDir)
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
            std::string_view dataDir,
            std::string_view userAgent,
            std::string_view infoFileUrl)
        : RemoteFilesLangManager{
            dataDir,
            traineddataExt,
            userAgent,
            infoFileUrl,
            getLocalLangs(dataDir)}
    {
    }

    bool shouldIgnoreLang(std::string_view langCode) const override
    {
        return isIgnoredLang(langCode);
    }

    std::string getLangName(std::string_view langCode) const override
    {
        return std::string{tesseract::getLangName(langCode)};
    }
};


}


bool hasLangManager()
{
    return true;
}


std::unique_ptr<ocr::LangManager> createLangManager(
    std::string_view dataDir,
    std::string_view userAgent,
    std::string_view infoFileUrl)
{
    return std::make_unique<LangManager>(
        dataDir, userAgent, infoFileUrl);
}


}
