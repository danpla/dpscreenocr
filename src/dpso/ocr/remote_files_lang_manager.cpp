
#include "ocr/remote_files_lang_manager.h"

#include <cassert>
#include <cerrno>
#include <cstring>

#include <jansson.h>

#include "dpso_net/download_file.h"
#include "dpso_net/error.h"
#include "dpso_net/get_data.h"

#include "dpso_utils/error.h"
#include "dpso_utils/os.h"
#include "dpso_utils/sha256_file.h"
#include "dpso_utils/str.h"

#include "ocr/lang_manager_error.h"


namespace dpso::ocr {
namespace {


void rethrowNetErrorAsLangManagerError(const char* message)
{
    try {
        throw;
    } catch (net::ConnectionError&) {
        throw LangManagerNetworkConnectionError{message};
    } catch (net::Error&) {
        throw LangManagerError{message};
    }
}


struct JsonRefDecrementer {
    void operator()(json_t* json) const
    {
        json_decref(json);
    }
};


using JsonUPtr = std::unique_ptr<json_t, JsonRefDecrementer>;


const json_t* get(const json_t* object, const char* key)
{
    if (const auto* obj = json_object_get(object, key))
        return obj;

    throw LangManagerError{str::printf("No \"%s\"", key)};
}


std::string getStr(const json_t* object, const char* key)
{
    if (const auto* obj = get(object, key);
            const auto* val = json_string_value(obj))
        return {val, json_string_length(obj)};

    throw LangManagerError{
        str::printf("\"%s\" is not a string", key)};
}


std::int64_t getInt(const json_t* object, const char* key)
{
    if (const auto* obj = get(object, key); json_is_integer(obj))
        return json_integer_value(obj);

    throw LangManagerError{
        str::printf("\"%s\" is not an integer", key)};
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
    return getLangName(getLangCode(langIdx).c_str());
}


LangManager::LangState RemoteFilesLangManager::getLangState(
    int langIdx) const
{
    return langInfos[langIdx].state;
}


void RemoteFilesLangManager::fetchExternalLangs()
{
    clearExternalLangs();

    for (const auto& externalLang : getExternalLangs(
            infoFileUrl.c_str(), userAgent.c_str()))
        if (!shouldIgnoreLang(externalLang.code.c_str()))
            addExternalLang(externalLang);
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
                    static_cast<float>(curSize)
                    / *totalSize
                    * 100;

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
    if (!dpsoMakeDirs(dataDir.c_str()))
        throw LangManagerError{str::printf(
            "Can't create directory \"%s\": %s",
            dataDir.c_str(), dpsoGetError())};

    auto& langInfo = langInfos[langIdx];

    assert(langInfo.sha256 != langInfo.externalSha256);
    assert(!langInfo.url.empty());

    const auto filePath = getFilePath(langIdx);

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
    langInfo.sha256 = langInfo.externalSha256;

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
    const auto filePath = getFilePath(langIdx);
    if (dpsoRemove(filePath.c_str()) != 0)
        throw LangManagerError{str::printf(
            "Can't remove \"%s\": %s",
            filePath.c_str(), std::strerror(errno))};

    if (auto& langInfo = langInfos[langIdx]; !langInfo.url.empty()) {
        langInfo.state = LangState::notInstalled;
        langInfo.sha256.clear();
    } else
        langInfos.erase(langInfos.begin() + langIdx);

    try {
        removeSha256File(filePath.c_str());
    } catch (Sha256FileError&) {
        // Ignore errors.
    }
}


std::vector<RemoteFilesLangManager::ExternalLangInfo>
RemoteFilesLangManager::parseJsonFileInfo(const char* jsonData)
{
    json_error_t jsonError;
    const JsonUPtr json{json_loads(jsonData, 0, &jsonError)};

    if (!json)
        throw LangManagerError{jsonError.text};

    if (!json_is_array(json.get()))
        throw LangManagerError{"Root is not an array"};

    std::vector<ExternalLangInfo> result;

    for (std::size_t i = 0; i < json_array_size(json.get()); ++i)
        try {
            const auto* fileInfo = json_array_get(json.get(), i);
            if (!json_is_object(fileInfo))
                throw LangManagerError{"Not an object"};

            result.push_back(
                {
                    getStr(fileInfo, "code"),
                    getStr(fileInfo, "sha256"),
                    getInt(fileInfo, "size"),
                    getStr(fileInfo, "url")
                });
        } catch (LangManagerError& e) {
            throw LangManagerError{str::printf(
                "Array item %zu: %s", i, e.what())};
        }

    return result;
}


std::vector<RemoteFilesLangManager::ExternalLangInfo>
RemoteFilesLangManager::getExternalLangs(
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
        return parseJsonFileInfo(jsonData.c_str());
    } catch (LangManagerError& e) {
        throw LangManagerError{str::printf(
            "Can't parse JSON info file from \"%s\": %s",
            infoFileUrl, e.what())};
    }
}


void RemoteFilesLangManager::clearExternalLangs()
{
    for (auto iter = langInfos.begin(); iter < langInfos.end();) {
        if (iter->state == LangState::notInstalled) {
            iter = langInfos.erase(iter);
            continue;
        }

        iter->state = LangState::installed;
        iter->externalSha256.clear();
        iter->url.clear();

        ++iter;
    }
}


void RemoteFilesLangManager::addExternalLang(
    const ExternalLangInfo& externalLang)
{
    for (auto& langInfo : langInfos) {
        if (langInfo.code != externalLang.code)
            continue;

        // Old external langs should be cleared before adding new
        // ones.
        assert(langInfo.state == LangState::installed);
        assert(langInfo.externalSha256.empty());
        assert(langInfo.url.empty());

        langInfo.externalSha256 = externalLang.sha256;
        langInfo.url = externalLang.url;

        const auto filePath = getFilePath(langInfo.code);

        // As a small optimization, check the file sizes first to
        // avoid calculating SHA-256 if the sizes are different.
        const auto fileSize = dpsoGetFileSize(filePath.c_str());
        if (fileSize == -1)
            throw LangManagerError{str::printf(
                "Can't get size of \"%s\": %s",
                filePath.c_str(), dpsoGetError())};

        if (fileSize != externalLang.size) {
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

        if (langInfo.sha256 != langInfo.externalSha256)
            langInfo.state = LangState::updateAvailable;

        return;
    }

    langInfos.push_back(
        {
            externalLang.code,
            LangState::notInstalled,
            {},
            externalLang.sha256,
            externalLang.url
        });
}


std::string RemoteFilesLangManager::getFilePath(
    const std::string& langCode) const
{
    return
        dataDir + *dpsoDirSeparators + getFileName(langCode.c_str());
}


std::string RemoteFilesLangManager::getFilePath(int langIdx) const
{
    return getFilePath(langInfos[langIdx].code);
}


}
