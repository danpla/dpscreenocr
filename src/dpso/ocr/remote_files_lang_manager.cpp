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


static std::int64_t getFileSize(const std::string& filePath)
{
    try {
        return os::getFileSize(filePath.c_str());
    } catch (os::Error& e) {
        throw LangManagerError{str::format(
            "Can't get size of \"{}\": {}", filePath, e.what())};
    }
}


RemoteFilesLangManager::RemoteFilesLangManager(
        const char* dataDir,
        const char* fileExt,
        const char* userAgent,
        const char* infoFileUrl,
        const std::vector<std::string>& localLangCodes)
    : dataDir{dataDir}
    , fileExt{fileExt}
    , userAgent{userAgent}
    , infoFileUrl{infoFileUrl}
{
    langInfos.reserve(localLangCodes.size());
    for (const auto& langCode : localLangCodes)
        langInfos.push_back(
            {
                langCode,
                LangState::installed,
                {-1, getFileSize(getFilePath(langCode))},
                {}});
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


RemoteFilesLangManager::LangSize
RemoteFilesLangManager::getLangSize(int langIdx) const
{
    return langInfos[langIdx].size;
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
        throw LangManagerError{str::format(
            "Can't create directory \"{}\": {}", dataDir, e.what())};
    }

    auto& langInfo = langInfos[langIdx];

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
        rethrowNetErrorAsLangManagerError(str::format(
            "Can't download \"{}\" to \"{}\": {}",
            langInfo.url,
            filePath,
            e.what()).c_str());
    }

    if (canceled)
        return;

    langInfo.state = LangState::installed;

    // Note that we can't assume that the file size or SHA-256 from
    // the JSON info is still relevant to the downloaded language,
    // because the files on the remote server can change at any time
    // after the JSON info is fetched.
    langInfo.size.local = getFileSize(filePath);

    // Remove a no longer relevant SHA-256 file in case we did an
    // update; it will be created if necessary the next time we check
    // if the language file is up to date.
    try {
        removeSha256File(filePath.c_str());
    } catch (Sha256FileError&) {
    }
}


void RemoteFilesLangManager::removeLang(int langIdx)
{
    const auto filePath = getFilePath(langInfos[langIdx].code);

    try {
        os::removeFile(filePath.c_str());
    } catch (os::Error& e) {
        throw LangManagerError{str::format(
            "Can't remove \"{}\": {}", filePath, e.what())};
    }

    if (auto& langInfo = langInfos[langIdx]; !langInfo.url.empty()) {
        langInfo.state = LangState::notInstalled;
        langInfo.size.local = -1;
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
            } catch (LangCodeError& e) {
                throw json::Error{str::format(
                    "Invalid code \"{}\": {}", code, e.what())};
            }

            result.push_back(
                {
                    code,
                    fileInfo.getStr("sha256"),
                    fileInfo.getInt("size"),
                    fileInfo.getStr("url")});
        } catch (json::Error& e) {
            throw json::Error{str::format(
                "File info at index {}: {}", i, e.what())};
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
        rethrowNetErrorAsLangManagerError(str::format(
            "Can't get data from \"{}\": {}",
            infoFileUrl, e.what()).c_str());
    }

    try {
        return parseJsonFileInfos(jsonData.c_str());
    } catch (json::Error& e) {
        throw LangManagerError{str::format(
            "Can't parse JSON info file from \"{}\": {}",
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
        iter->size.external = -1;
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

    assert(langInfo.size.external == -1);
    langInfo.size.external = remoteLangInfo.size;

    assert(langInfo.url.empty());
    langInfo.url = remoteLangInfo.url;

    const auto filePath = getFilePath(langInfo.code);

    // As an optimization, don't calculate SHA-256 if file sizes are
    // different.
    if (langInfo.size.local != remoteLangInfo.size) {
        langInfo.state = LangState::updateAvailable;
        return;
    }

    try {
        if (getSha256HexDigestWithCaching(filePath.c_str())
                != remoteLangInfo.sha256)
            langInfo.state = LangState::updateAvailable;
    } catch (Sha256FileError& e) {
        throw LangManagerError{str::format(
            "Can't get SHA-256 of \"{}\": {}", filePath, e.what())};
    }
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
            {remoteLangInfo.size, -1},
            remoteLangInfo.url});
}


std::string RemoteFilesLangManager::getFilePath(
    const std::string& langCode) const
{
    return dataDir + *os::dirSeparators + langCode + fileExt;
}


}
