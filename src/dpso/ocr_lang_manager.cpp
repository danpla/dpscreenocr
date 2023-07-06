
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

#include "dpso_utils/error.h"
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
    // State is the only field that can be changed from the background
    // thread (during installation).
    dpso::Synchronized<LangState> state;
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
                false
            });

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
        if (iter->state.getLock().get() == LangState::notInstalled) {
            iter = langs.erase(iter);
            continue;
        }

        iter->installMark = false;
        iter->state = LangState::installed;

        ++iter;
    }
}


class LangOpCanceled : public std::runtime_error {
public:
    LangOpCanceled()
        : std::runtime_error("")
    {
    }
};


class LangOpExecutor {
public:
    class Control {
    public:
        Control()
            : Control{{}, {}}
        {
        }

        Control(
                const std::shared_future<LangOpStatus>& future,
                const std::shared_ptr<dpso::Synchronized<bool>>&
                    cancelFlag)
            : future{future}
            , status{
                future.valid()
                    ? DpsoOcrLangOpStatusCodeProgress
                    : DpsoOcrLangOpStatusCodeNone
                , {}}
            , cancelFlag{cancelFlag}
        {
        }

        LangOpStatus getStatus() const
        {
            if (status.code == DpsoOcrLangOpStatusCodeProgress
                    && future.valid()
                    && future.wait_for(std::chrono::seconds{})
                        == std::future_status::ready) {
                status = future.get();
                assert(
                    status.code != DpsoOcrLangOpStatusCodeProgress);
            }

            return status;
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
        mutable LangOpStatus status;
        std::shared_ptr<dpso::Synchronized<bool>> cancelFlag;
    };

    explicit LangOpExecutor(dpso::ocr::LangManager& langManager)
        : langManager{langManager}
    {
    }

    ~LangOpExecutor()
    {
        if (cancelFlag)
            *cancelFlag = true;

        if (future.valid())
            future.wait();
    }

    void setUserAgent(const char* newUserAgent)
    {
        userAgent = newUserAgent;
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

        langManager.setUserAgent(userAgent.getLock().get().c_str());

        cancelFlag = std::make_shared<dpso::Synchronized<bool>>();

        future = std::async(
            std::launch::async,
            [this, fn = std::forward<Fn>(fn)]() -> LangOpStatus
            {
                try {
                    fn(langManager, *cancelFlag);
                } catch (std::exception& e) {
                    return {catchAsStatusCode(), e.what()};
                }

                return {DpsoOcrLangOpStatusCodeSuccess, {}};
            });

        return getControl();
    }
private:
    dpso::Synchronized<std::string> userAgent;
    dpso::ocr::LangManager& langManager;

    std::shared_ptr<dpso::Synchronized<bool>> cancelFlag;
    std::shared_future<LangOpStatus> future;

    static DpsoOcrLangOpStatusCode catchAsStatusCode()
    {
        try {
            throw;
        } catch (LangOpCanceled&) {
            return DpsoOcrLangOpStatusCodeNone;
        } catch (dpso::ocr::LangManagerNetworkConnectionError&) {
            return DpsoOcrLangOpStatusCodeNetworkConnectionError;
        } catch (std::exception&) {
            // dpso::ocr::LangManagerError and other exceptions
            return DpsoOcrLangOpStatusCodeGenericError;
        } catch (...) {
            assert(false);
            throw;
        }
    }
};


struct Impl {
    std::shared_ptr<dpso::ocr::DataLock> dataLock;

    std::vector<Lang> langs;

    std::unique_ptr<dpso::ocr::LangManager> langManager;

    LangOpExecutor::Control fetchControl;

    LangOpExecutor::Control installControl;
    dpso::Synchronized<DpsoOcrLangInstallProgress> installProgress;

    // A cache for C API functions in order extend the lifetime of
    // DpsoOcrLangOpStatus::errorText.
    mutable struct {
        LangOpStatus fetchStatus;
        LangOpStatus installStatus;
    } apiCache;

    std::unique_ptr<LangOpExecutor> langOpExecutor;
};


}


// We hide impl under getImpl() methods to propagate constness of
// the DpsoOcrLangManager pointer is to impl.
struct DpsoOcrLangManager {
    explicit DpsoOcrLangManager(const std::shared_ptr<Impl>& impl)
        : impl{impl}
    {
    }

    const Impl& getImpl() const
    {
        return *impl;
    }

    Impl& getImpl()
    {
        return *impl;
    }
private:
    std::shared_ptr<Impl> impl;
};


DpsoOcrLangManager* dpsoOcrLangManagerCreate(
    int engineIdx, const char* dataDir)
{
    if (engineIdx < 0
            || static_cast<std::size_t>(engineIdx)
                >= dpso::ocr::Engine::getCount()) {
        dpsoSetError("engineIdx is out of bounds");
        return nullptr;
    }

    const auto& ocrEngine = dpso::ocr::Engine::get(engineIdx);

    const auto& engineId = ocrEngine.getInfo().id;

    struct ImplCacheEntry {
        std::string engineId;
        std::string dataDir;
        std::weak_ptr<Impl> impl;
    };

    static std::vector<ImplCacheEntry> implCache;

    for (const auto& entry : implCache) {
        if (entry.engineId != engineId || entry.dataDir != dataDir)
            continue;

        if (auto impl = entry.impl.lock())
            return new DpsoOcrLangManager{impl};

        assert(false);
    }

    std::shared_ptr<Impl> impl{
        new Impl{},
        [engineId, dataDir = std::string{dataDir}]
        (Impl* impl)
        {
            for (auto iter = implCache.begin();
                    iter < implCache.end();
                    ++iter)
                if (iter->engineId == engineId
                        && iter->dataDir == dataDir) {
                    implCache.erase(iter);
                    break;
                }

            delete impl;
        }};

    impl->dataLock = dpso::ocr::DataLock::get(
        engineId.c_str(), dataDir);

    try {
        impl->langManager = ocrEngine.createLangManager(dataDir);
    } catch (dpso::ocr::LangManagerError& e) {
        dpsoSetError("%s", e.what());
        return nullptr;
    }

    reloadLangs(impl->langs, *impl->langManager);

    impl->langOpExecutor = std::make_unique<LangOpExecutor>(
        *impl->langManager);

    implCache.push_back({engineId, dataDir, impl});

    return new DpsoOcrLangManager{impl};
}


void dpsoOcrLangManagerDelete(DpsoOcrLangManager* langManager)
{
    delete langManager;
}


void dpsoOcrLangManagerSetUserAgent(
    DpsoOcrLangManager* langManager, const char* userAgent)
{
    if (langManager)
        langManager->getImpl().langOpExecutor->setUserAgent(
            userAgent);
}


int dpsoOcrLangManagerGetNumLangs(
    const DpsoOcrLangManager* langManager)
{
    return langManager ? langManager->getImpl().langs.size() : 0;
}


int dpsoOcrLangManagerGetLangIdx(
    const DpsoOcrLangManager* langManager, const char* langCode)
{
    if (!langManager)
        return -1;

    const auto& langs = langManager->getImpl().langs;

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
    -> decltype(&langManager->getImpl().langs[0])
{
    if (!langManager
            || langIdx < 0
            || static_cast<std::size_t>(langIdx)
                >= langManager->getImpl().langs.size())
        return {};

    return &langManager->getImpl().langs[langIdx];
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

    switch (lang->state.getLock().get()) {
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


bool dpsoOcrLangOpStatusIsError(DpsoOcrLangOpStatusCode code)
{
    return code >= DpsoOcrLangOpStatusCodeGenericError;
}


static void setLangOpActiveError()
{
    dpsoSetError("An operation is active");
}


bool dpsoOcrLangManagerStartFetchExternalLangs(
    DpsoOcrLangManager* langManager)
{
    if (!langManager) {
        dpsoSetError("langManager is null");
        return false;
    }

    auto& impl = langManager->getImpl();

    if (impl.langOpExecutor->getControl().getStatus().code
            == DpsoOcrLangOpStatusCodeProgress) {
        setLangOpActiveError();
        return false;
    }

    clearExternalLangs(impl.langs);

    impl.fetchControl = impl.langOpExecutor->execute(
        [](
            dpso::ocr::LangManager& langManager,
            const dpso::Synchronized<bool>& /* cancelRequested */)
        {
            langManager.fetchExternalLangs();
        });

    return true;
}


void dpsoOcrLangManagerGetFetchExternalLangsStatus(
    const DpsoOcrLangManager* langManager,
    DpsoOcrLangOpStatus* status)
{
    if (!langManager || !status)
        return;

    const auto& impl = langManager->getImpl();

    impl.apiCache.fetchStatus = impl.fetchControl.getStatus();
    *status = toPublic(impl.apiCache.fetchStatus);
}


bool dpsoOcrLangManagerLoadFetchedExternalLangs(
    DpsoOcrLangManager* langManager)
{
    if (!langManager) {
        dpsoSetError("langManager is null");
        return false;
    }

    auto& impl = langManager->getImpl();

    impl.fetchControl.waitToFinish();

    if (impl.langOpExecutor->getControl().getStatus().code
            == DpsoOcrLangOpStatusCodeProgress) {
        setLangOpActiveError();
        return false;
    }

    const auto fetchStatus = impl.fetchControl.getStatus();

    if (fetchStatus.code == DpsoOcrLangOpStatusCodeNone) {
        dpsoSetError("Fetching wasn't started");
        return false;
    }

    if (dpsoOcrLangOpStatusIsError(fetchStatus.code)) {
        dpsoSetError(
            "Fetching failed: %s", fetchStatus.errorText.c_str());
        return false;
    }

    assert(fetchStatus.code == DpsoOcrLangOpStatusCodeSuccess);

    reloadLangs(impl.langs, *impl.langManager);

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
    if (lang && lang->state.getLock().get() != LangState::installed)
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
    dpso::ocr::LangManager& langManager,
    const dpso::Synchronized<bool>& cancelRequested,
    const std::vector<int>& langIndices,
    std::vector<Lang>& langs,
    dpso::Synchronized<DpsoOcrLangInstallProgress>& installProgress)
{
    for (std::size_t i = 0; i < langIndices.size(); ++i) {
        if (cancelRequested.getLock().get())
            throw LangOpCanceled{};

        const auto langIdx = langIndices[i];
        auto& lang = langs[langIdx];

        const auto baseLangIdx = getLangIdx(
            langManager, lang.code.c_str());
        assert(baseLangIdx);

        langManager.installLang(
            *baseLangIdx,
            [&](int progress)
            {
                installProgress = {
                    langIdx,
                    progress,
                    static_cast<int>(i + 1),
                    static_cast<int>(langIndices.size())
                };

                return !cancelRequested.getLock().get();
            });

        lang.state = langManager.getLangState(*baseLangIdx);
    }
}


bool dpsoOcrLangManagerStartInstall(DpsoOcrLangManager* langManager)
{
    if (!langManager) {
        dpsoSetError("langManager is null");
        return false;
    }

    auto& impl = langManager->getImpl();

    if (impl.langOpExecutor->getControl().getStatus().code
            == DpsoOcrLangOpStatusCodeProgress) {
        setLangOpActiveError();
        return false;
    }

    std::vector<int> langIndices;

    for (std::size_t i = 0; i < impl.langs.size(); ++i) {
        if (!impl.langs[i].installMark)
            continue;

        langIndices.push_back(i);

        impl.langs[i].installMark = false;
    }

    if (langIndices.empty()) {
        dpsoSetError("No languages are marked for installation");
        return false;
    }

    impl.installProgress = {
        langIndices[0], 0, 1, static_cast<int>(langIndices.size())
    };

    impl.installControl = impl.langOpExecutor->execute(
        [&, langIndices = std::move(langIndices)]
        (
            dpso::ocr::LangManager& langManager,
            const dpso::Synchronized<bool>& cancelRequested)
        {
            const dpso::ScopeExit resetProgress{
                [&]
                {
                    impl.installProgress = {-1, 0, 0, 0};
                }};

            installLangs(
                langManager,
                cancelRequested,
                langIndices,
                impl.langs,
                impl.installProgress);
        });

    return true;
}


void dpsoOcrLangManagerGetInstallProgress(
    const DpsoOcrLangManager* langManager,
    DpsoOcrLangInstallProgress* progress)
{
    if (langManager && progress)
        *progress =
            langManager->getImpl().installProgress.getLock().get();
}


void dpsoOcrLangManagerGetInstallStatus(
    const DpsoOcrLangManager* langManager,
    DpsoOcrLangOpStatus* status)
{
    if (!langManager || !status)
        return;

    const auto& impl = langManager->getImpl();

    impl.apiCache.installStatus = impl.installControl.getStatus();
    *status = toPublic(impl.apiCache.installStatus);
}


void dpsoOcrLangManagerCancelInstall(DpsoOcrLangManager* langManager)
{
    if (!langManager)
        return;

    auto& impl = langManager->getImpl();
    impl.installControl.requestCancel();
    impl.installControl.waitToFinish();
}


bool dpsoOcrLangManagerRemoveLang(
    DpsoOcrLangManager* langManager, int langIdx)
{
    if (!langManager) {
        dpsoSetError("langManager is null");
        return false;
    }

    auto& impl = langManager->getImpl();

    if (impl.langOpExecutor->getControl().getStatus().code
            == DpsoOcrLangOpStatusCodeProgress) {
        setLangOpActiveError();
        return false;
    }

    auto* lang = getLang(langManager, langIdx);
    if (!lang) {
        dpsoSetError("langIdx is out of bounds");
        return false;
    }

    if (lang->state.getLock().get() == LangState::notInstalled)
        return true;

    const auto baseLangIdx = getLangIdx(
        *impl.langManager, lang->code.c_str());

    // Fetching external languages cannot implicitly "remove" locally
    // available ones from LangManager. Since we rejected the language
    // with the "not installed" state, we now have a guarantee that
    // LangManager includes all languages from Impl::langs, even if
    // Impl::langs wasn't synchronized with LangManager via
    // dpsoOcrLangManagerLoadFetchedExternalLangs().
    assert(baseLangIdx);

    // Only LangManager knows if a language with the "installed" state
    // will turn to "not installed" or be removed from the list. We
    // will detect this implicitly by the list length so that we don't
    // have to reload whole Impl::langs.
    const auto numLangsBefore = impl.langManager->getNumLangs();

    try {
        impl.langManager->removeLang(*baseLangIdx);
    } catch (dpso::ocr::LangManagerError& e) {
        dpsoSetError("%s", e.what());
        return false;
    }

    if (impl.langManager->getNumLangs() == numLangsBefore)
        lang->state = impl.langManager->getLangState(*baseLangIdx);
    else
        impl.langs.erase(impl.langs.begin() + langIdx);

    return true;
}
