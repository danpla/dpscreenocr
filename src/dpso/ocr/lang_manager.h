
#pragma once

#include <functional>
#include <string>


namespace dpso::ocr {


class LangManager {
public:
    // The progress is in percents from 0 to 100. If the file size is
    // unknown, the method will be called at fixed intervals with -1
    // progress value.
    //
    // Returns false to terminate installation.
    using ProgressHandler = std::function<bool(int progress)>;

    enum class LangState {
        notInstalled,
        installed,
        updateAvailable
    };

    virtual ~LangManager() = default;

    virtual void setUserAgent(const char* newUserAgent) = 0;

    virtual int getNumLangs() const = 0;

    virtual std::string getLangCode(int langIdx) const = 0;

    virtual std::string getLangName(int langIdx) const = 0;

    virtual LangState getLangState(int langIdx) const = 0;

    virtual void fetchExternalLangs() = 0;

    virtual void installLang(
        int langIdx, const ProgressHandler& progressHandler) = 0;

    virtual void removeLang(int langIdx) = 0;
};


}
