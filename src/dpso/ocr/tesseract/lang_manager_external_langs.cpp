
#include "ocr/tesseract/lang_manager_external_langs.h"

#include <chrono>
#include <cstring>
#include <memory>

#include <jansson.h>
#include <tesseract/baseapi.h>

#include "dpso_net/error.h"
#include "dpso_net/get_data.h"

#include "dpso_utils/str.h"
#include "dpso_utils/version_cmp.h"

#include "ocr/lang_manager_error.h"
#include "ocr/tesseract/lang_manager_error_utils.h"
#include "ocr/tesseract/lang_utils.h"


namespace dpso::ocr::tesseract {
namespace {


struct JsonRefDecrementer {
    void operator()(json_t* json) const
    {
        json_decref(json);
    }
};


using JsonUPtr = std::unique_ptr<json_t, JsonRefDecrementer>;


const auto* const gitHubApiReposUrl =
    "https://api.github.com/repos/tesseract-ocr/tessdata_fast";


std::string parseJsonTagInfo(const json_t* tagInfo)
{
    if (!json_is_object(tagInfo))
        throw LangManagerError{"Not an object"};

    const auto* nameObj = json_object_get(tagInfo, "name");
    if (!nameObj)
        throw LangManagerError{"No \"name\""};

    const auto* name = json_string_value(nameObj);
    if (!name)
        throw LangManagerError{"\"name\" is not a string"};

    return name;
}


std::vector<std::string> parseJsonTags(const char* jsonData)
{
    std::vector<std::string> result;

    json_error_t error;
    const JsonUPtr json{json_loads(jsonData, 0, &error)};

    if (!json)
        throw LangManagerError{error.text};

    if (!json_is_array(json.get()))
        throw LangManagerError{"Root is not an array"};

    for (std::size_t i = 0; i < json_array_size(json.get()); ++i)
        try {
            result.push_back(
                parseJsonTagInfo(json_array_get(json.get(), i)));
        } catch (LangManagerError& e) {
            throw LangManagerError{str::printf(
                "Array item %zu: %s", i, e.what())};
        }

    return result;
}


// Get a Git data repository tag <= the current Tesseract version.
std::string getRepoTag(const char* userAgent)
{
    const auto tagsUrl = std::string{gitHubApiReposUrl} +  "/tags";

    std::string jsonData;
    try {
        jsonData = net::getData(tagsUrl.c_str(), userAgent);
    } catch (net::Error& e) {
        rethrowNetErrorAsLangManagerError(str::printf(
            "Can't get data from \"%s\": %s",
            tagsUrl.c_str(), e.what()).c_str());
    }

    std::vector<std::string> tags;
    try {
        tags = parseJsonTags(jsonData.c_str());
    } catch (LangManagerError& e) {
        throw LangManagerError{str::printf(
            "Can't parse JSON tags: %s", e.what())};
    }

    const VersionCmp targetVersion{
        ::tesseract::TessBaseAPI::Version()};

    VersionCmp bestVersion;
    std::string bestTag;
    for (const auto& tag : tags) {
        const auto* versionStr = tag.c_str();

        // Some older tags start with "v", like "v1.0rc1"
        if (*versionStr == 'v')
            ++versionStr;

        const VersionCmp curVersion{versionStr};

        if (targetVersion < curVersion)
            continue;

        if (bestVersion < curVersion) {
            bestVersion = curVersion;
            bestTag = tag;
        }
    }

    if (bestTag.empty())
        throw LangManagerError{str::printf(
            "Can't find a suitable Git tag for version %s",
            ::tesseract::TessBaseAPI::Version())};

    return bestTag;
}


void parseJsonFileInfo(
    const json_t* fileInfo, std::vector<ExternalLangInfo>& dst)
{
    if (!json_is_object(fileInfo))
        throw LangManagerError{"Not an object"};

    const auto* pathObj = json_object_get(fileInfo, "path");
    if (!pathObj)
        throw LangManagerError{"No \"path\""};

    const auto* path = json_string_value(pathObj);
    if (!path)
        throw LangManagerError{"\"path\" is not a string"};

    const auto* extPos = std::strrchr(path, '.');
    if (!extPos || std::strcmp(extPos, traineddataExt) != 0)
        return;

    const std::string langCode{path, extPos};
    if (isIgnoredLang(langCode.c_str()))
        return;

    const auto* sizeObj = json_object_get(fileInfo, "size");
    if (!sizeObj)
        throw LangManagerError{"No \"size\""};

    const auto size = json_integer_value(sizeObj);

    const auto* downloadUrlObj = json_object_get(
        fileInfo, "download_url");
    if (!downloadUrlObj)
        throw LangManagerError{"No \"download_url\""};

    const auto* downloadUrl = json_string_value(downloadUrlObj);
    if (!downloadUrl)
        throw LangManagerError{"\"download_url\" is not a string"};

    dst.push_back({langCode, size, downloadUrl});
}


std::vector<ExternalLangInfo> parseJsonRepoContents(
    const char* jsonData)
{
    json_error_t error;
    const JsonUPtr json{json_loads(jsonData, 0, &error)};

    if (!json)
        throw LangManagerError{error.text};

    if (!json_is_array(json.get()))
        throw LangManagerError{"Root is not an array"};

    std::vector<ExternalLangInfo> result;

    for (std::size_t i = 0; i < json_array_size(json.get()); ++i)
        try {
            parseJsonFileInfo(json_array_get(json.get(), i), result);
        } catch (LangManagerError& e) {
            throw LangManagerError{str::printf(
                "Array item %zu: %s", i, e.what())};
        }

    return result;
}


// As of this writing, GitHub API has a rate limit of 60 requests per
// hour for an unauthenticated user. Since we keep the cache in RAM
// rather than on drive, the user may still exceed the limit by
// launching the application more than 30 times per hour (we make 2
// requests per query: for the tag list and for repository contents),
// but we consider this an unlikely case.
struct Cache {
    using Clock = std::chrono::steady_clock;

    static constexpr std::chrono::hours lifetime{1};

    std::vector<ExternalLangInfo> infos;
    Clock::time_point time;
} cache;


}


std::vector<ExternalLangInfo> getExternalLangs(const char* userAgent)
{
    const auto curTime = Cache::Clock::now();
    if (!cache.infos.empty()
            && curTime - cache.time < Cache::lifetime)
        return cache.infos;

    const auto contentsUrl =
        std::string{gitHubApiReposUrl}
        + "/contents?ref="
        + getRepoTag(userAgent);

    std::string jsonData;
    try {
        jsonData = net::getData(contentsUrl.c_str(), userAgent);
    } catch (net::Error& e) {
        rethrowNetErrorAsLangManagerError(str::printf(
            "Can't get data from \"%s\": %s",
            contentsUrl.c_str(), e.what()).c_str());
    }

    try {
        cache.infos = parseJsonRepoContents(jsonData.c_str());
    } catch (LangManagerError& e) {
        throw LangManagerError{str::printf(
            "Can't parse JSON repo contents: %s", e.what())};
    }

    cache.time = curTime;
    return cache.infos;
}


}
