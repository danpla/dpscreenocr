#pragma once

#include <cstdint>
#include <functional>
#include <string>


namespace dpso::ocr {


class LangManager {
public:
    // The progress is in percents from 0 to 100. If the file size is
    // unknown, the function is called at fixed intervals with -1
    // progress value.
    //
    // Returns false to terminate installation.
    using ProgressHandler = std::function<bool(int progress)>;

    virtual ~LangManager() = default;

    virtual int getNumLangs() const = 0;

    virtual std::string getLangCode(int langIdx) const = 0;

    virtual std::string getLangName(int langIdx) const = 0;

    // See DpsoOcrLangState
    enum class LangState {
        notInstalled,
        installed,
        updateAvailable
    };

    virtual LangState getLangState(int langIdx) const = 0;

    // See DpsoOcrLangSize
    struct LangSize {
        std::int64_t external;
        std::int64_t local;
    };

    virtual LangSize getLangSize(int langIdx) const = 0;

    // See dpsoOcrLangManagerStartFetchExternalLangs() and
    // dpsoOcrLangManagerLoadFetchedExternalLangs().
    //
    // Before doing the actual fetching, the method should remove the
    // previously fetched languages from the list, so that in case of
    // an error LangManager will be in the same state as when it was
    // created (i.e., containing only locally available languages with
    // the LangState::installed state).
    //
    // Throws LangManagerError.
    virtual void fetchExternalLangs() = 0;

    // See dpsoOcrLangManagerStartInstall().
    //
    // The method will not be called for a language with the
    // LangState::installed state.
    //
    // Throws LangManagerError.
    virtual void installLang(
        int langIdx, const ProgressHandler& progressHandler) = 0;

    // See dpsoOcrLangManagerRemoveLang().
    //
    // The method will not be called for a language with the
    // LangState::notInstalled state.
    //
    // Throws LangManagerError.
    virtual void removeLang(int langIdx) = 0;
};


}
