#include "ocr_lang_manager.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "dpso_utils/error_set.h"
#include "dpso_utils/scope_exit.h"
#include "dpso_utils/synchronized.h"

#include "ocr/engine.h"
#include "ocr/lang_manager.h"
#include "ocr/lang_manager_error.h"
#include "ocr_data_lock.h"


namespace {


using LangState = dpso::ocr::LangManager::LangState;


struct LangOpStatus {
    DpsoOcrLangOpStatusCode code;
    std::string errorText;
};


DpsoOcrLangOpStatus toPublic(const LangOpStatus& status)
{
    return {status.code, status.errorText.c_str()};
}


struct Lang {
    std::string code;
    std::string name;
    // State and size are the only fields that can be changed from the
    // background thread (during installation).
    dpso::Synchronized<LangState> state;
    dpso::Synchronized<dpso::ocr::LangManager::LangSize> size;
    bool installMark;
};


void reloadLangs(
    std::vector<Lang>& langs,
    const dpso::ocr::LangManager& langManager)
{
    langs.clear();

    langs.reserve(langManager.getNumLangs());
    for (auto i = 0; i < langManager.getNumLangs(); ++i)
        langs.push_back(
            {
                langManager.getLangCode(i),
                langManager.getLangName(i),
                langManager.getLangState(i),
                langManager.getLangSize(i),
                false});

    std::sort(
        langs.begin(), langs.end(),
        [](const Lang& a, const Lang& b)
        {
            return a.code < b.code;
        });
}


void clearExternalLangs(std::vector<Lang>& langs)
{
    for (auto iter = langs.begin(); iter < langs.end();) {
        if (*iter->state.getLock() == LangState::notInstalled) {
            iter = langs.erase(iter);
            continue;
        }

        iter->installMark = false;
        iter->state = LangState::installed;
        iter->size.getLock()->external = -1;

        ++iter;
    }
}


class LangOpExecutor {
public:
    class OpCanceled : public std::runtime_error {
    public:
        OpCanceled()
            : std::runtime_error("")
        {
        }
    };

    class Control {
    public:
        Control() = default;

        Control(
                const std::shared_future<LangOpStatus>& future,
                const std::shared_ptr<dpso::Synchronized<bool>>&
                    cancelFlag)
            : future{future}
            , cancelFlag{cancelFlag}
        {
        }

        LangOpStatus getStatus() const
        {
            if (!future.valid())
                return {DpsoOcrLangOpStatusCodeNone, {}};

            if (future.wait_for(std::chrono::seconds{})
                    == std::future_status::timeout)
                return {DpsoOcrLangOpStatusCodeProgress, {}};

            return future.get();
        }

        void requestCancel()
        {
            if (cancelFlag)
                *cancelFlag = true;
        }

        void waitToFinish()
        {
            if (future.valid())
                future.wait();
        }
    private:
        std::shared_future<LangOpStatus> future;
        std::shared_ptr<dpso::Synchronized<bool>> cancelFlag;
    };

    ~LangOpExecutor()
    {
        if (cancelFlag)
            *cancelFlag = true;

        if (future.valid())
            future.wait();
    }

    Control getControl() const
    {
        return {future, cancelFlag};
    }

    template<typename Fn>
    Control execute(Fn&& fn)
    {
        if (future.valid())
            future.wait();

        cancelFlag = std::make_shared<dpso::Synchronized<bool>>();

        future = std::async(
            std::launch::async,
            [this, fn = std::forward<Fn>(fn)]() -> LangOpStatus
            {
                try {
                    fn(*cancelFlag);
                } catch (std::runtime_error& e) {
                    return {catchAsStatusCode(), e.what()};
                }

                return {DpsoOcrLangOpStatusCodeSuccess, {}};
            });

        return getControl();
    }
private:
    std::shared_ptr<dpso::Synchronized<bool>> cancelFlag;
    std::shared_future<LangOpStatus> future;

    static DpsoOcrLangOpStatusCode catchAsStatusCode()
    {
        try {
            throw;
        } catch (LangOpExecutor::OpCanceled&) {
            return DpsoOcrLangOpStatusCodeNone;
        } catch (dpso::ocr::LangManagerNetworkConnectionError&) {
            return DpsoOcrLangOpStatusCodeNetworkConnectionError;
        } catch (std::runtime_error&) {
            // dpso::ocr::LangManagerError and other exceptions
            return DpsoOcrLangOpStatusCodeGenericError;
        } catch (...) {
            assert(false);
            throw;
        }
    }
};


}


struct DpsoOcrLangManager {
    dpso::ocr::DataLock dataLock;

    std::vector<Lang> langs;

    std::unique_ptr<dpso::ocr::LangManager> langManager;

    LangOpExecutor::Control fetchControl;

    LangOpExecutor::Control installControl;
    dpso::Synchronized<DpsoOcrLangInstallProgress> installProgress;

    // A cache for C API functions to extend the lifetime of
    // DpsoOcrLangOpStatus::errorText.
    mutable struct {
        LangOpStatus fetchStatus;
        LangOpStatus installStatus;
    } apiCache;

    LangOpExecutor langOpExecutor;
};


DpsoOcrLangManager* dpsoOcrLangManagerCreate(
    int engineIdx,
    const char* dataDir,
    const char* userAgent,
    const char* infoFileUrl)
{
    if (engineIdx < 0
            || static_cast<std::size_t>(engineIdx)
                >= dpso::ocr::Engine::getCount()) {
        dpso::setError("engineIdx is out of bounds");
        return nullptr;
    }

    const auto& ocrEngine = dpso::ocr::Engine::get(engineIdx);

    const auto& engineId = ocrEngine.getInfo().id;

    auto langManager = std::make_unique<DpsoOcrLangManager>();

    try {
        langManager->dataLock = dpso::ocr::DataLock{
            engineId.c_str(), dataDir};
    } catch (dpso::ocr::DataLock::DataLockedError& e) {
        dpso::setError("{}", e.what());
        return nullptr;
    }

    try {
        langManager->langManager = ocrEngine.createLangManager(
            dataDir, userAgent, infoFileUrl);
    } catch (dpso::ocr::LangManagerError& e) {
        dpso::setError("{}", e.what());
        return nullptr;
    }

    reloadLangs(langManager->langs, *langManager->langManager);

    return langManager.release();
}


void dpsoOcrLangManagerDelete(DpsoOcrLangManager* langManager)
{
    delete langManager;
}


int dpsoOcrLangManagerGetNumLangs(
    const DpsoOcrLangManager* langManager)
{
    return langManager ? langManager->langs.size() : 0;
}


int dpsoOcrLangManagerGetLangIdx(
    const DpsoOcrLangManager* langManager, const char* langCode)
{
    if (!langManager)
        return -1;

    const auto& langs = langManager->langs;

    const auto iter = std::lower_bound(
        langs.begin(), langs.end(), langCode,
        [&](const Lang& lang, const char* langCode)
        {
            return lang.code < langCode;
        });

    if (iter != langs.end() && iter->code == langCode)
        return iter - langs.begin();

    return -1;
}


template<typename T>
static auto getLang(T* langManager, int langIdx)
    -> decltype(&langManager->langs[0])
{
    if (!langManager
            || langIdx < 0
            || static_cast<std::size_t>(langIdx)
                >= langManager->langs.size())
        return {};

    return &langManager->langs[langIdx];
}


const char* dpsoOcrLangManagerGetLangCode(
    const DpsoOcrLangManager* langManager, int langIdx)
{
    const auto* lang = getLang(langManager, langIdx);
    return lang ? lang->code.c_str() : "";
}


const char* dpsoOcrLangManagerGetLangName(
    const DpsoOcrLangManager* langManager, int langIdx)
{
    const auto* lang = getLang(langManager, langIdx);
    return lang ? lang->name.c_str() : "";
}


DpsoOcrLangState dpsoOcrLangManagerGetLangState(
    const DpsoOcrLangManager* langManager, int langIdx)
{
    const auto* lang = getLang(langManager, langIdx);
    if (!lang)
        return {};

    switch (*lang->state.getLock()) {
    case LangState::notInstalled:
        return DpsoOcrLangStateNotInstalled;
    case LangState::installed:
        return DpsoOcrLangStateInstalled;
    case LangState::updateAvailable:
        return DpsoOcrLangStateUpdateAvailable;
    }

    assert(false);
    return {};
}


void dpsoOcrLangManagerGetLangSize(
    const DpsoOcrLangManager* langManager,
    int langIdx,
    DpsoOcrLangSize* langSize)
{
    const auto* lang = getLang(langManager, langIdx);
    if (!lang || !langSize)
        return;

    const auto size = *lang->size.getLock();
    *langSize = {size.external, size.local};
}


bool dpsoOcrLangOpStatusIsError(DpsoOcrLangOpStatusCode code)
{
    return code >= DpsoOcrLangOpStatusCodeGenericError;
}


static void setLangOpActiveError()
{
    dpso::setError("An operation is active");
}


bool dpsoOcrLangManagerStartFetchExternalLangs(
    DpsoOcrLangManager* langManager)
{
    if (!langManager) {
        dpso::setError("langManager is null");
        return false;
    }

    if (langManager->langOpExecutor.getControl().getStatus().code
            == DpsoOcrLangOpStatusCodeProgress) {
        setLangOpActiveError();
        return false;
    }

    clearExternalLangs(langManager->langs);

    langManager->fetchControl = langManager->langOpExecutor.execute(
        [=](const dpso::Synchronized<bool>& /* cancelRequested */)
        {
            langManager->langManager->fetchExternalLangs();
        });

    return true;
}


void dpsoOcrLangManagerGetFetchExternalLangsStatus(
    const DpsoOcrLangManager* langManager,
    DpsoOcrLangOpStatus* status)
{
    if (!langManager || !status)
        return;

    langManager->apiCache.fetchStatus =
        langManager->fetchControl.getStatus();
    *status = toPublic(langManager->apiCache.fetchStatus);
}


bool dpsoOcrLangManagerLoadFetchedExternalLangs(
    DpsoOcrLangManager* langManager)
{
    if (!langManager) {
        dpso::setError("langManager is null");
        return false;
    }

    langManager->fetchControl.waitToFinish();

    if (langManager->langOpExecutor.getControl().getStatus().code
            == DpsoOcrLangOpStatusCodeProgress) {
        setLangOpActiveError();
        return false;
    }

    const auto fetchStatus = langManager->fetchControl.getStatus();

    if (fetchStatus.code == DpsoOcrLangOpStatusCodeNone) {
        dpso::setError("Fetching wasn't started");
        return false;
    }

    if (dpsoOcrLangOpStatusIsError(fetchStatus.code)) {
        dpso::setError("Fetching failed: {}", fetchStatus.errorText);
        return false;
    }

    assert(fetchStatus.code == DpsoOcrLangOpStatusCodeSuccess);

    reloadLangs(langManager->langs, *langManager->langManager);

    return true;
}


bool dpsoOcrLangManagerGetInstallMark(
    const DpsoOcrLangManager* langManager, int langIdx)
{
    const auto* lang = getLang(langManager, langIdx);
    return lang && lang->installMark;
}


void dpsoOcrLangManagerSetInstallMark(
    DpsoOcrLangManager* langManager, int langIdx, bool installMark)
{
    auto* lang = getLang(langManager, langIdx);
    if (lang && *lang->state.getLock() != LangState::installed)
        lang->installMark = installMark;
}


static std::optional<int> getLangIdx(
    const dpso::ocr::LangManager& langManager, const char* langCode)
{
    for (int i = 0; i < langManager.getNumLangs(); ++i)
        if (langManager.getLangCode(i) == langCode)
            return i;

    return {};
}


static void installLangs(
    DpsoOcrLangManager& langManager,
    const std::vector<int>& langIndices,
    const dpso::Synchronized<bool>& cancelRequested)
{
    for (std::size_t i = 0; i < langIndices.size(); ++i) {
        if (*cancelRequested.getLock())
            throw LangOpExecutor::OpCanceled{};

        const auto langIdx = langIndices[i];
        auto& lang = langManager.langs[langIdx];

        const auto baseLangIdx = getLangIdx(
            *langManager.langManager, lang.code.c_str());
        assert(baseLangIdx);

        langManager.langManager->installLang(
            *baseLangIdx,
            [&](int progress)
            {
                langManager.installProgress = {
                    langIdx,
                    progress,
                    static_cast<int>(i + 1),
                    static_cast<int>(langIndices.size())};

                return !*cancelRequested.getLock();
            });

        lang.state = langManager.langManager->getLangState(
            *baseLangIdx);
        lang.size = langManager.langManager->getLangSize(
            *baseLangIdx);
    }
}


bool dpsoOcrLangManagerStartInstall(DpsoOcrLangManager* langManager)
{
    if (!langManager) {
        dpso::setError("langManager is null");
        return false;
    }

    if (langManager->langOpExecutor.getControl().getStatus().code
            == DpsoOcrLangOpStatusCodeProgress) {
        setLangOpActiveError();
        return false;
    }

    std::vector<int> langIndices;

    for (std::size_t i = 0; i < langManager->langs.size(); ++i) {
        if (!langManager->langs[i].installMark)
            continue;

        langIndices.push_back(i);

        langManager->langs[i].installMark = false;
    }

    if (langIndices.empty()) {
        dpso::setError("No languages are marked for installation");
        return false;
    }

    langManager->installProgress = {
        langIndices[0], 0, 1, static_cast<int>(langIndices.size())};

    langManager->installControl = langManager->langOpExecutor.execute(
        [
            langManager,
            langIndices = std::move(langIndices)]
        (const dpso::Synchronized<bool>& cancelRequested)
        {
            const dpso::ScopeExit resetProgress{
                [&]
                {
                    langManager->installProgress = {-1, 0, 0, 0};
                }};

            installLangs(*langManager, langIndices, cancelRequested);
        });

    return true;
}


void dpsoOcrLangManagerGetInstallProgress(
    const DpsoOcrLangManager* langManager,
    DpsoOcrLangInstallProgress* progress)
{
    if (langManager && progress)
        *progress = *langManager->installProgress.getLock();
}


void dpsoOcrLangManagerGetInstallStatus(
    const DpsoOcrLangManager* langManager,
    DpsoOcrLangOpStatus* status)
{
    if (!langManager || !status)
        return;

    langManager->apiCache.installStatus =
        langManager->installControl.getStatus();
    *status = toPublic(langManager->apiCache.installStatus);
}


void dpsoOcrLangManagerCancelInstall(DpsoOcrLangManager* langManager)
{
    if (!langManager)
        return;

    auto& ic = langManager->installControl;
    ic.requestCancel();
    ic.waitToFinish();
}


bool dpsoOcrLangManagerRemoveLang(
    DpsoOcrLangManager* langManager, int langIdx)
{
    if (!langManager) {
        dpso::setError("langManager is null");
        return false;
    }

    if (langManager->langOpExecutor.getControl().getStatus().code
            == DpsoOcrLangOpStatusCodeProgress) {
        setLangOpActiveError();
        return false;
    }

    auto* lang = getLang(langManager, langIdx);
    if (!lang) {
        dpso::setError("langIdx is out of bounds");
        return false;
    }

    if (*lang->state.getLock() == LangState::notInstalled)
        return true;

    const auto baseLangIdx = getLangIdx(
        *langManager->langManager, lang->code.c_str());

    // Fetching external languages cannot implicitly "remove" locally
    // available ones from LangManager. Since we rejected the language
    // with the "not installed" state, we now have a guarantee that
    // LangManager includes all languages from the
    // DpsoOcrLangManager::langs list, even if there was no
    // dpsoOcrLangManagerLoadFetchedExternalLangs() call.
    assert(baseLangIdx);

    // Only LangManager knows if a language with the "installed" state
    // will turn to "not installed" or be removed from the list. We
    // will detect this implicitly by the list length so that we don't
    // have to reload whole DpsoOcrLangManager::langs.
    const auto numLangsBefore =
        langManager->langManager->getNumLangs();

    try {
        langManager->langManager->removeLang(*baseLangIdx);
    } catch (dpso::ocr::LangManagerError& e) {
        dpso::setError("{}", e.what());
        return false;
    }

    if (langManager->langManager->getNumLangs() == numLangsBefore)
        lang->state = langManager->langManager->getLangState(
            *baseLangIdx);
    else
        langManager->langs.erase(
            langManager->langs.begin() + langIdx);

    return true;
}
