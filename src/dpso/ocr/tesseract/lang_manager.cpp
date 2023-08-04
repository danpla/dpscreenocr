
#include "ocr/tesseract/lang_manager.h"

#include "ocr/lang_manager_error.h"
#include "ocr/remote_files_lang_manager.h"
#include "ocr/tesseract/lang_names.h"
#include "ocr/tesseract/lang_utils.h"


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


class TesseractLangManager : public RemoteFilesLangManager {
public:
    TesseractLangManager(
            const char* dataDir,
            const char* userAgent,
            const char* infoFileUrl)
        : RemoteFilesLangManager{
            dataDir,
            userAgent,
            infoFileUrl,
            getLocalLangs(dataDir)}
    {
    }

    bool shouldIgnoreLang(const char* langCode) const override
    {
        return isIgnoredLang(langCode);
    }

    std::string getFileName(const char* langCode) const override
    {
        return std::string{langCode} + traineddataExt;
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


std::unique_ptr<LangManager> createLangManager(
    const char* dataDir,
    const char* userAgent,
    const char* infoFileUrl)
{
    return std::make_unique<TesseractLangManager>(
        dataDir, userAgent, infoFileUrl);
}


}
