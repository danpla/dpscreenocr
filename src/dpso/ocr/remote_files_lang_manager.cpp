
#include "ocr/remote_files_lang_manager.h"

#include <cassert>

#include "dpso_json/json.h"

#include "dpso_net/download_file.h"
#include "dpso_net/error.h"
#include "dpso_net/get_data.h"

#include "dpso_utils/os.h"
#include "dpso_utils/sha256_file.h"
#include "dpso_utils/str.h"

#include "ocr/lang_code_validator.h"
#include "ocr/lang_manager_error.h"


namespace dpso::ocr {


static void rethrowNetErrorAsLangManagerError(const char* message)
{
    try {
        throw;
    } catch (net::ConnectionError&) {
        throw LangManagerNetworkConnectionError{message};
    } catch (net::Error&) {
        throw LangManagerError{message};
    }
}


RemoteFilesLangManager::RemoteFilesLangManager(
        const char* dataDir,
        const char* userAgent,
        const char* infoFileUrl,
        const std::vector<std::string>& localLangCodes)
    : dataDir{dataDir}
    , userAgent{userAgent}
    , infoFileUrl{infoFileUrl}
{
    langInfos.reserve(localLangCodes.size());
    for (const auto& langCode : localLangCodes)
        langInfos.push_back(
            {langCode, LangState::installed, {}, {}, {}});
}


bool RemoteFilesLangManager::shouldIgnoreLang(
    const char* langCode) const
{
    (void)langCode;
    return false;
}


int RemoteFilesLangManager::getNumLangs() const
{
    return langInfos.size();
}


std::string RemoteFilesLangManager::getLangCode(int langIdx) const
{
    return langInfos[langIdx].code;
}


std::string RemoteFilesLangManager::getLangName(int langIdx) const
{
    return getLangName(langInfos[langIdx].code.c_str());
}


LangManager::LangState RemoteFilesLangManager::getLangState(
    int langIdx) const
{
    return langInfos[langIdx].state;
}


void RemoteFilesLangManager::fetchExternalLangs()
{
    clearRemoteLangs();

    for (const auto& remoteLangInfo : getRemoteLangs(
            infoFileUrl.c_str(), userAgent.c_str()))
        if (!shouldIgnoreLang(remoteLangInfo.code.c_str()))
            addRemoteLang(remoteLangInfo);
}


static net::DownloadProgressHandler makeDownloadProgressHandler(
    const LangManager::ProgressHandler& progressHandler,
    bool& canceled)
{
    return [&, lastProgress = -1](
        std::int64_t curSize,
        std::optional<std::int64_t> totalSize) mutable
    {
        if (!progressHandler)
            return true;

        auto progress = -1;

        if (totalSize) {
            if (*totalSize == 0)
                progress = 100;
            else
                progress =
                    static_cast<float>(curSize) / *totalSize * 100;

            if (progress == lastProgress)
                return true;

            lastProgress = progress;
        }

        canceled = !progressHandler(progress);
        return !canceled;
    };
}


void RemoteFilesLangManager::installLang(
    int langIdx, const ProgressHandler& progressHandler)
{
    try {
        os::makeDirs(dataDir.c_str());
    } catch (os::Error& e) {
        throw LangManagerError{str::printf(
            "Can't create directory \"%s\": %s",
            dataDir.c_str(), e.what())};
    }

    auto& langInfo = langInfos[langIdx];

    assert(langInfo.sha256 != langInfo.remoteSha256);
    assert(!langInfo.url.empty());

    const auto filePath = getFilePath(langInfo.code);

    bool canceled{};

    try {
        net::downloadFile(
            langInfo.url.c_str(),
            userAgent.c_str(),
            filePath.c_str(),
            makeDownloadProgressHandler(progressHandler, canceled));
    } catch (net::Error& e) {
        rethrowNetErrorAsLangManagerError(str::printf(
            "Can't download \"%s\" to \"%s\": %s",
            langInfo.url.c_str(),
            filePath.c_str(),
            e.what()).c_str());
    }

    if (canceled)
        return;

    langInfo.state = LangState::installed;
    langInfo.sha256 = langInfo.remoteSha256;

    // Even though we know the digest in advance, we cannot save it
    // before the language file is downloaded, as this would
    // prematurely overwrite the existing digest file in case of a
    // language update.
    try {
        saveSha256File(filePath.c_str(), langInfo.sha256.c_str());
    } catch (Sha256FileError&) {
        // Ignore errors, as the language file is already downloaded
        // anyway, and the previous one is overwritten in case of an
        // update.
    }
}


void RemoteFilesLangManager::removeLang(int langIdx)
{
    const auto filePath = getFilePath(langInfos[langIdx].code);

    try {
        os::removeFile(filePath.c_str());
    } catch (os::Error& e) {
        throw LangManagerError{str::printf(
            "Can't remove \"%s\": %s", filePath.c_str(), e.what())};
    }

    if (auto& langInfo = langInfos[langIdx]; !langInfo.url.empty()) {
        langInfo.state = LangState::notInstalled;
        langInfo.sha256.clear();
    } else
        langInfos.erase(langInfos.begin() + langIdx);

    try {
        removeSha256File(filePath.c_str());
    } catch (Sha256FileError&) {
    }
}


std::vector<RemoteFilesLangManager::RemoteLangInfo>
RemoteFilesLangManager::parseJsonFileInfos(const char* jsonData)
{
    const auto fileInfos = json::Array::load(jsonData);

    std::vector<RemoteLangInfo> result;
    result.reserve(fileInfos.getSize());

    for (std::size_t i = 0; i < fileInfos.getSize(); ++i) {
        const auto fileInfo = fileInfos.getObject(i);

        try {
            const auto code = fileInfo.getStr("code");
            try {
                validateLangCode(code.c_str());
            } catch (InvalidLangCodeError& e) {
                throw json::Error{str::printf(
                    "Invalid code \"%s\": %s",
                    code.c_str(), e.what())};
            }

            result.push_back(
                {
                    code,
                    fileInfo.getStr("sha256"),
                    fileInfo.getInt("size"),
                    fileInfo.getStr("url")
                });
        } catch (json::Error& e) {
            throw json::Error{str::printf(
                "File info at index %zu: %s", i, e.what())};
        }
    }

    return result;
}


std::vector<RemoteFilesLangManager::RemoteLangInfo>
RemoteFilesLangManager::getRemoteLangs(
    const char* infoFileUrl, const char* userAgent)
{
    std::string jsonData;
    try {
        jsonData = net::getData(infoFileUrl, userAgent);
    } catch (net::Error& e) {
        rethrowNetErrorAsLangManagerError(str::printf(
            "Can't get data from \"%s\": %s",
            infoFileUrl, e.what()).c_str());
    }

    try {
        return parseJsonFileInfos(jsonData.c_str());
    } catch (json::Error& e) {
        throw LangManagerError{str::printf(
            "Can't parse JSON info file from \"%s\": %s",
            infoFileUrl, e.what())};
    }
}


void RemoteFilesLangManager::clearRemoteLangs()
{
    for (auto iter = langInfos.begin(); iter < langInfos.end();) {
        if (iter->state == LangState::notInstalled) {
            iter = langInfos.erase(iter);
            continue;
        }

        iter->state = LangState::installed;
        iter->remoteSha256.clear();
        iter->url.clear();

        ++iter;
    }
}


void RemoteFilesLangManager::mergeRemoteLang(
    LangInfo& langInfo, const RemoteLangInfo& remoteLangInfo)
{
    assert(langInfo.code == remoteLangInfo.code);

    // Old remote langs should be cleared before adding new ones.
    assert(langInfo.state == LangState::installed);
    assert(langInfo.remoteSha256.empty());
    assert(langInfo.url.empty());

    langInfo.remoteSha256 = remoteLangInfo.sha256;
    langInfo.url = remoteLangInfo.url;

    const auto filePath = getFilePath(langInfo.code);

    // As an optimization, compare file sizes first to avoid
    // calculating SHA-256 if they are different.
    std::int64_t fileSize{};
    try {
        fileSize = os::getFileSize(filePath.c_str());
    } catch (os::Error& e) {
        throw LangManagerError{str::printf(
            "Can't get size of \"%s\": %s",
            filePath.c_str(), e.what())};
    }

    if (fileSize != remoteLangInfo.size) {
        langInfo.state = LangState::updateAvailable;
        return;
    }

    if (langInfo.sha256.empty())
        try {
            langInfo.sha256 = getSha256HexDigestWithCaching(
                filePath.c_str());
        } catch (Sha256FileError& e) {
            throw LangManagerError{str::printf(
                "Can't get SHA-256 of \"%s\": %s",
                filePath.c_str(), e.what())};
        }

    if (langInfo.sha256 != langInfo.remoteSha256)
        langInfo.state = LangState::updateAvailable;
}


void RemoteFilesLangManager::addRemoteLang(
    const RemoteLangInfo& remoteLangInfo)
{
    for (auto& langInfo : langInfos)
        if (langInfo.code == remoteLangInfo.code) {
            mergeRemoteLang(langInfo, remoteLangInfo);
            return;
        }

    langInfos.push_back(
        {
            remoteLangInfo.code,
            LangState::notInstalled,
            {},
            remoteLangInfo.sha256,
            remoteLangInfo.url
        });
}


std::string RemoteFilesLangManager::getFilePath(
    const std::string& langCode) const
{
    return
        dataDir + *os::dirSeparators + getFileName(langCode.c_str());
}


}
