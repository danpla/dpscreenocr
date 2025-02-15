#pragma once

#include <cstdint>
#include <vector>

#include "engine/lang_manager.h"


namespace dpso::ocr {


// A language manager that uses files from a remote server.
class RemoteFilesLangManager : public LangManager {
public:
    // See dpsoOcrLangManagerCreate() for the details.
    RemoteFilesLangManager(
        const char* dataDir,
        const char* fileExt,
        const char* userAgent,
        const char* infoFileUrl,
        const std::vector<std::string>& localLangCodes);

    // This method is called for every remote language to check if it
    // should be excluded from the LangManager list. The default
    // implementation returns false.
    virtual bool shouldIgnoreLang(const char* langCode) const;

    virtual std::string getLangName(const char* langCode) const = 0;

    int getNumLangs() const override;

    std::string getLangCode(int langIdx) const override;

    std::string getLangName(int langIdx) const override;

    LangState getLangState(int langIdx) const override;

    LangSize getLangSize(int langIdx) const override;

    void fetchExternalLangs() override;

    void installLang(
        int langIdx, const ProgressHandler& progressHandler) override;

    void removeLang(int langIdx) override;
private:
    struct LangInfo {
        std::string code;
        LangState state;
        LangSize size;
        std::string url;
    };

    struct RemoteLangInfo {
        std::string code;
        std::string sha256;
        std::int64_t size;
        std::string url;
    };

    std::string dataDir;
    std::string fileExt;
    std::string userAgent;
    std::string infoFileUrl;
    std::vector<LangInfo> langInfos;

    static std::vector<RemoteLangInfo> parseJsonFileInfos(
        const char* jsonData);
    static std::vector<RemoteLangInfo> getRemoteLangs(
        const char* infoFileUrl, const char* userAgent);

    void clearRemoteLangs();
    void mergeRemoteLang(
        LangInfo& langInfo, const RemoteLangInfo& remoteLangInfo);
    void addRemoteLang(const RemoteLangInfo& remoteLangInfo);
    std::string getFilePath(const std::string& langCode) const;
};


}
