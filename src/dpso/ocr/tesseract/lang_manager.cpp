
#include "ocr/tesseract/lang_manager.h"

#include <cassert>
#include <cerrno>
#include <cstring>
#include <vector>

#include "dpso_net/download_file.h"
#include "dpso_net/error.h"

#include "dpso_utils/os.h"
#include "dpso_utils/str.h"

#include "ocr/lang_manager_error.h"
#include "ocr/tesseract/lang_manager_error_utils.h"
#include "ocr/tesseract/lang_manager_external_langs.h"
#include "ocr/tesseract/lang_names.h"
#include "ocr/tesseract/lang_utils.h"


namespace dpso::ocr::tesseract {
namespace {


class TesseractLangManager : public LangManager {
public:
    explicit TesseractLangManager(const char* dataDir)
        : dataDir{dataDir}
        , userAgent{}
        , langInfos{}
    {
        for (const auto& langCode : getAvailableLangs(dataDir))
            langInfos.push_back({langCode, LangState::installed, {}});
    }

    void setUserAgent(const char* newUserAgent)
    {
        userAgent = newUserAgent;
    }

    int getNumLangs() const override
    {
        return langInfos.size();
    }

    std::string getLangCode(int langIdx) const override
    {
        return langInfos[langIdx].code;
    }

    std::string getLangName(int langIdx) const override
    {
        const auto* name = tesseract::getLangName(
            langInfos[langIdx].code.c_str());
        return name ? name : "";
    }

    LangState getLangState(int langIdx) const override
    {
        return langInfos[langIdx].state;
    }

    void fetchExternalLangs() override;

    void installLang(
        int langIdx, ProgressHandler progressHandler) override;

    void removeLang(int langIdx) override;
private:
    struct LangInfo {
        std::string code;
        LangState state;
        std::string url;
    };

    std::string dataDir;
    std::string userAgent;
    std::vector<LangInfo> langInfos;

    void clearExternalLangs();
    void addExternalLang(const ExternalLangInfo& externalLang);
    std::string getFilePath(const std::string& code) const;
    std::string getFilePath(int langIdx) const;
};


void TesseractLangManager::clearExternalLangs()
{
    for (auto iter = langInfos.begin(); iter < langInfos.end();) {
        if (iter->state == LangState::notInstalled) {
            iter = langInfos.erase(iter);
            continue;
        }

        iter->state = LangState::installed;
        iter->url.clear();

        ++iter;
    }
}


void TesseractLangManager::addExternalLang(
    const ExternalLangInfo& externalLang)
{
    for (auto& langInfo : langInfos)
        if (langInfo.code == externalLang.code) {
            // Old external langs should be cleared before adding new
            // ones.
            assert(langInfo.state == LangState::installed);
            assert(langInfo.url.empty());

            // TODO: Using the file size is clearly a lousy way to
            // check for file changes. It may be OK as a last resort
            // when we only have plain HTTP (i.e. "Content-Length"),
            // but we currently use the GitHub API, which provides a
            // Git blob hash.
            // https://git-scm.com/book/en/v2/Git-Internals-Git-Objects#_object_storage
            const auto fileSize = dpsoGetFileSize(
                getFilePath(langInfo.code).c_str());
            if (fileSize != -1 && fileSize != externalLang.size)
                langInfo.state = LangState::updateAvailable;

            langInfo.url = externalLang.url;
            return;
        }

    langInfos.push_back(
        {externalLang.code,
            LangState::notInstalled,
            externalLang.url});
}


void TesseractLangManager::fetchExternalLangs()
{
    clearExternalLangs();

    for (const auto& externalLang
            : getExternalLangs(userAgent.c_str()))
        addExternalLang(externalLang);
}


std::string TesseractLangManager::getFilePath(
    const std::string& code) const
{
    return dataDir + *dpsoDirSeparators + code + traineddataExt;
}


std::string TesseractLangManager::getFilePath(int langIdx) const
{
    return getFilePath(langInfos[langIdx].code);
}


struct DownloadProgressHandler {
    explicit DownloadProgressHandler(
            LangManager::ProgressHandler& progressHandler)
        : progressHandler{progressHandler}
        , lastProgress{-1}
        , canceled{}
    {
    }

    bool operator()(
        std::uint64_t curSize,
        std::optional<std::uint64_t> totalSize)
    {
        if (!progressHandler)
            return true;

        auto progress = -1;

        if (totalSize) {
            if (*totalSize == 0)
                progress = 100;
            else
                progress =
                    static_cast<float>(curSize)
                    / *totalSize
                    * 100;

            if (progress == lastProgress)
                return true;

            lastProgress = progress;
        }

        canceled = !progressHandler(progress);

        return !canceled;
    }

    LangManager::ProgressHandler& progressHandler;
    int lastProgress;
    bool canceled;
};


void TesseractLangManager::installLang(
    int langIdx, ProgressHandler progressHandler)
{
    if (langInfos[langIdx].state == LangState::installed)
        return;

    assert(!langInfos[langIdx].url.empty());

    const auto filePath = getFilePath(langIdx);
    const auto& url = langInfos[langIdx].url;
    DownloadProgressHandler downloadProgressHandler{progressHandler};

    try {
        net::downloadFile(
            url.c_str(),
            userAgent.c_str(),
            filePath.c_str(),
            downloadProgressHandler);
    } catch (net::Error& e) {
        rethrowNetErrorAsLangManagerError(str::printf(
            "Can't download \"%s\" to \"%s\": %s",
            url.c_str(),
            filePath.c_str(),
            e.what()).c_str());
    }

    if (!downloadProgressHandler.canceled)
        langInfos[langIdx].state = LangState::installed;
}


void TesseractLangManager::removeLang(int langIdx)
{
    if (langInfos[langIdx].state == LangState::notInstalled)
        return;

    const auto filePath = getFilePath(langIdx);

    if (dpsoRemove(filePath.c_str()) != 0)
        throw LangManagerError{str::printf(
            "Can't remove \"%s\": %s",
            filePath.c_str(),
            std::strerror(errno))};

    if (!langInfos[langIdx].url.empty())
        langInfos[langIdx].state = LangState::notInstalled;
    else
        langInfos.erase(langInfos.begin() + langIdx);
}


}


bool hasLangManager()
{
    return true;
}


std::unique_ptr<LangManager> createLangManager(const char* dataDir)
{
    return std::make_unique<TesseractLangManager>(dataDir);
}


}
