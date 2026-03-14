#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>


namespace dpso::ocr {


class LangManager;
class Recognizer;


// See DpsoOcrEngineInfo
struct EngineInfo {
    std::string id;
    std::string name;
    std::string version;

    // See DpsoOcrEngineDataDirPreference
    enum class DataDirPreference {
        noDataDir,
        preferDefault,
        preferExplicit,
    };

    DataDirPreference dataDirPreference;
    bool hasLangManager;
};


class Engine {
public:
    static const Engine& get(std::size_t idx);
    static std::size_t getCount();

    virtual ~Engine() = default;

    virtual const EngineInfo& getInfo() const = 0;

    // Throws RecognizerError
    virtual std::unique_ptr<Recognizer> createRecognizer(
        std::string_view dataDir) const = 0;

    // See dpsoOcrLangManagerCreate(). Throws LangManagerError.
    virtual std::unique_ptr<LangManager> createLangManager(
        std::string_view dataDir,
        std::string_view userAgent,
        std::string_view infoFileUrl) const = 0;
};


}
