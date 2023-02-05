
#include "ocr.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "dpso_utils/error.h"
#include "dpso_utils/progress_tracker.h"
#include "dpso_utils/strftime.h"
#include "dpso_utils/timing.h"

#include "backend/backend.h"
#include "backend/screenshot_error.h"
#include "backend/screenshot.h"
#include "geometry_c.h"
#include "geometry.h"
#include "img.h"
#include "ocr_engine/ocr_engine_creator.h"
#include "ocr_engine/ocr_engine.h"


// A note on concurrency
//
// Since we don't require any thread safety guarantees from OcrEngine,
// we never use it from multiple threads. Only OcrEngine::recognize()
// is called after DpsoOcr is created; everything else is cached. An
// alternative would be to protect OcrEngine access with a mutex, but
// it's impractical for functions like dpsoOcrGetLangCode() to block
// during OCR since it takes significant time.


static dpso::backend::Backend* backend;


const std::chrono::milliseconds threadIdleTime{10};


namespace {


struct Lang {
    std::string code;
    std::string name;
    // The public API gives languages sorted by codes, while in
    // OcrEngine they may be in arbitrary order. A Lang at the index
    // from the public API refers to the OcrEngine language at
    // Lang::idx.
    int idx;
    bool isActive;
};


struct Job {
    std::unique_ptr<dpso::backend::Screenshot> screenshot;
    std::vector<int> langIndices;
    dpso::ocr::OcrFeatures ocrFeatures;
    std::string timestamp;
};


struct JobResult {
    dpso::ocr::OcrResult ocrResult;
    std::string timestamp;
};


// Link between main and background threads.
struct Link {
    mutable std::mutex lock;

    std::queue<Job> jobQueue;
    bool jobActive;

    bool waitingForResults;
    DpsoOcrProgressCallback waitingProgressCallback;
    void* waitingUserData;

    DpsoOcrProgress progress;

    std::vector<JobResult> results;

    bool terminateJobs;
    bool terminateThread;

    bool jobsPending() const
    {
        return !jobQueue.empty() || jobActive;
    }
};


#define LINK_LOCK(L) std::lock_guard linkLockGuard{L.lock}


}


int dpsoOcrGetNumEngines(void)
{
    return dpso::ocr::OcrEngineCreator::getCount();
}


void dpsoOcrGetEngineInfo(int idx, DpsoOcrEngineInfo* info)
{
    if (idx < 0
            || static_cast<std::size_t>(idx)
                >= dpso::ocr::OcrEngineCreator::getCount()
            || !info)
        return;

    const auto& internalInfo = dpso::ocr::OcrEngineCreator::get(
        idx).getInfo();

    DpsoOcrEngineDataDirPreference dataDirPreference{};
    switch (internalInfo.dataDirPreference) {
    case dpso::ocr::OcrEngineInfo::DataDirPreference::noDataDir:
        dataDirPreference = DpsoOcrEngineDataDirPreferenceNoDataDir;
        break;
    case dpso::ocr::OcrEngineInfo::DataDirPreference::preferDefault:
        dataDirPreference =
            DpsoOcrEngineDataDirPreferencePreferDefault;
        break;
    case dpso::ocr::OcrEngineInfo::DataDirPreference::preferExplicit:
        dataDirPreference =
            DpsoOcrEngineDataDirPreferencePreferExplicit;
        break;
    }

    *info = {
        internalInfo.id.c_str(),
        internalInfo.name.c_str(),
        internalInfo.version.c_str(),
        dataDirPreference
    };
}


struct DpsoOcr {
    std::unique_ptr<dpso::ocr::OcrEngine> engine;

    std::string defaultLangCode;
    std::vector<Lang> langs;
    int numActiveLangs;

    Link link;
    std::thread thread;

    std::vector<std::uint8_t> imgBuffers[3];
    bool dumpDebugImage;

    std::vector<JobResult> fetchedResults;
    std::vector<DpsoOcrJobResult> returnedResults;
};


static void cacheLangs(DpsoOcr& ocr)
{
    ocr.langs.clear();
    ocr.langs.reserve(ocr.engine->getNumLangs());

    for (int i = 0; i < ocr.engine->getNumLangs(); ++i)
        ocr.langs.push_back(
            {
                ocr.engine->getLangCode(i),
                ocr.engine->getLangName(i),
                i,
                false
            });

    std::sort(
        ocr.langs.begin(), ocr.langs.end(),
        [&](const Lang& a, const Lang& b)
        {
            return a.code < b.code;
        });
}


static void threadLoop(DpsoOcr* ocr);


DpsoOcr* dpsoOcrCreate(const DpsoOcrArgs* ocrArgs)
{
    if (!ocrArgs) {
        dpsoSetError("ocrArgs is null");
        return nullptr;
    }

    if (ocrArgs->engineIdx < 0
            || static_cast<std::size_t>(ocrArgs->engineIdx)
                >= dpso::ocr::OcrEngineCreator::getCount()) {
        dpsoSetError("ocrArgs->engineIdx is out of bounds");
        return nullptr;
    }

    const auto& ocrEngineCreator = dpso::ocr::OcrEngineCreator::get(
        ocrArgs->engineIdx);

    // We don't use OcrUPtr here because dpsoOcrDelete() expects
    // a joinable thread.
    auto ocr = std::make_unique<DpsoOcr>();

    try {
        ocr->engine = ocrEngineCreator.create({ocrArgs->dataDir});
    } catch (dpso::ocr::OcrEngineError& e) {
        dpsoSetError("Can't create OCR engine: %s", e.what());
        return nullptr;
    }

    ocr->defaultLangCode = ocr->engine->getDefaultLangCode();
    cacheLangs(*ocr);

    ocr->thread = std::thread(threadLoop, ocr.get());

    const auto* dumpDebugImageEnvVar = std::getenv(
        "DPSO_DUMP_DEBUG_IMAGE");
    ocr->dumpDebugImage =
        dumpDebugImageEnvVar
        && std::strcmp(dumpDebugImageEnvVar, "0") != 0;

    return ocr.release();
}


void dpsoOcrDelete(DpsoOcr* ocr)
{
    if (!ocr)
        return;

    dpsoOcrTerminateJobs(ocr);

    {
        LINK_LOCK(ocr->link);
        ocr->link.terminateThread = true;
    }
    assert(ocr->thread.joinable());
    ocr->thread.join();

    delete ocr;
}


int dpsoOcrGetNumLangs(const DpsoOcr* ocr)
{
    return ocr ? ocr->langs.size() : 0;
}


const char* dpsoOcrGetLangCode(const DpsoOcr* ocr, int langIdx)
{
    if (!ocr
            || langIdx < 0
            || static_cast<std::size_t>(langIdx) >= ocr->langs.size())
        return "";

    return ocr->langs[langIdx].code.c_str();
}


const char* dpsoOcrGetDefaultLangCode(const DpsoOcr* ocr)
{
    return ocr ? ocr->defaultLangCode.c_str() : "";
}


const char* dpsoOcrGetLangName(const DpsoOcr* ocr, int langIdx)
{
    if (!ocr
            || langIdx < 0
            || static_cast<std::size_t>(langIdx) >= ocr->langs.size()
            || ocr->langs[langIdx].name.empty())
        return nullptr;

    return ocr->langs[langIdx].name.c_str();
}


int dpsoOcrGetLangIdx(const DpsoOcr* ocr, const char* langCode)
{
    if (!ocr)
        return -1;

    const auto iter = std::lower_bound(
        ocr->langs.begin(), ocr->langs.end(), langCode,
        [&](const Lang& lang, const char* langCode)
        {
            return lang.code < langCode;
        });

    if (iter != ocr->langs.end() && iter->code == langCode)
        return iter - ocr->langs.begin();

    return -1;
}


bool dpsoOcrGetLangIsActive(const DpsoOcr* ocr, int langIdx)
{
    return
        ocr
        && langIdx >= 0
        && static_cast<std::size_t>(langIdx) < ocr->langs.size()
        && ocr->langs[langIdx].isActive;
}


void dpsoOcrSetLangIsActive(
    DpsoOcr* ocr, int langIdx, bool newIsActive)
{
    if (!ocr
            || langIdx < 0
            || static_cast<std::size_t>(langIdx) >= ocr->langs.size()
            || newIsActive == ocr->langs[langIdx].isActive)
        return;

    ocr->langs[langIdx].isActive = newIsActive;

    if (newIsActive)
        ++ocr->numActiveLangs;
    else
        --ocr->numActiveLangs;
}


int dpsoOcrGetNumActiveLangs(const DpsoOcr* ocr)
{
    return ocr ? ocr->numActiveLangs : 0;
}


static std::vector<int> getActiveLangIndices(const DpsoOcr& ocr)
{
    std::vector<int> result;
    result.reserve(ocr.numActiveLangs);

    for (const auto& lang : ocr.langs)
        if (lang.isActive)
            result.push_back(lang.idx);

    return result;
}


static std::string createTimestamp()
{
    const auto time = std::time(nullptr);
    if (const auto* tm = std::localtime(&time))
        return dpso::strftime("%Y-%m-%d %H:%M:%S", tm);

    return {};
}


static dpso::ocr::OcrImage prepareScreenshot(
    DpsoOcr& ocr,
    const dpso::backend::Screenshot& screenshot,
    dpso::ProgressTracker& progressTracker)
{
    const auto imageScale = 4;
    const auto bufferW = screenshot.getWidth() * imageScale;
    const auto bufferH = screenshot.getHeight() * imageScale;
    const auto bufferPitch = bufferW;

    for (auto& buffer : ocr.imgBuffers)
        buffer.resize(bufferH * bufferPitch);

    START_TIMING(screenshotGetData);
    screenshot.getGrayscaleData(
        ocr.imgBuffers[0].data(), bufferPitch);
    END_TIMING(
        screenshotGetData,
        "screenshot.getGrayscaleData (%ix%i px)",
        screenshot.getWidth(), screenshot.getHeight());

    dpso::ProgressTracker localProgressTracker(2, &progressTracker);

    localProgressTracker.advanceJob();
    START_TIMING(imageResizing);
    dpso::img::resize(
        ocr.imgBuffers[0].data(),
        screenshot.getWidth(), screenshot.getHeight(), bufferPitch,
        ocr.imgBuffers[1].data(),
        bufferW, bufferH, bufferPitch,
        &localProgressTracker);
    END_TIMING(
        imageResizing,
        "Image resizing (%ix%i px -> %ix%i px, x%i)",
        screenshot.getWidth(), screenshot.getHeight(),
        bufferW, bufferH,
        imageScale);

    const auto unsharpMaskRadius = 10;

    localProgressTracker.advanceJob();
    START_TIMING(unsharpMasking);
    dpso::img::unsharpMask(
        ocr.imgBuffers[1].data(), bufferPitch,
        ocr.imgBuffers[0].data(), bufferPitch,
        ocr.imgBuffers[2].data(), bufferPitch,
        bufferW, bufferH,
        unsharpMaskRadius,
        1.0f,
        &localProgressTracker);
    END_TIMING(
        unsharpMasking,
        "Unsharp masking (radius=%i, %ix%i px)",
        unsharpMaskRadius, bufferW, bufferH);

    localProgressTracker.finish();

    return {ocr.imgBuffers[0].data(), bufferW, bufferH, bufferPitch};
}


namespace {


struct OcrProgressHandler : dpso::ocr::OcrProgressHandler {
    OcrProgressHandler(
            DpsoOcr& ocr,
            dpso::ProgressTracker& progressTracker)
        : ocr{ocr}
        , progressTracker{progressTracker}
    {
    }

    bool operator()(int progress) override
    {
        progressTracker.update(progress / 100.0f);

        auto& link = ocr.link;

        bool waitingForResults;
        {
            LINK_LOCK(link);
            waitingForResults = link.waitingForResults;
        }

        if (waitingForResults && link.waitingProgressCallback)
            link.waitingProgressCallback(&ocr, link.waitingUserData);

        LINK_LOCK(link);
        return !link.terminateJobs;
    }

    DpsoOcr& ocr;
    dpso::ProgressTracker& progressTracker;
};


struct ProgressHandler : dpso::ProgressTracker::ProgressHandler {
    explicit ProgressHandler(DpsoOcr& ocr)
        : ocr{ocr}
    {
    }

    void operator()(float progress) override
    {
        LINK_LOCK(ocr.link);
        ocr.link.progress.curJobProgress = progress * 100;
    }

    DpsoOcr& ocr;
};


}


static void processJob(DpsoOcr& ocr, const Job& job)
{
    assert(job.screenshot);
    assert(!job.langIndices.empty());

    ProgressHandler progressHandler{ocr};
    dpso::ProgressTracker progressTracker{2, progressHandler};

    progressTracker.advanceJob();
    const auto ocrImage = prepareScreenshot(
        ocr, *job.screenshot, progressTracker);

    if (ocr.dumpDebugImage)
        dpso::img::savePgm(
            "dpso_debug.pgm",
            ocrImage.data,
            ocrImage.width, ocrImage.height, ocrImage.pitch);

    progressTracker.advanceJob();

    OcrProgressHandler ocrProgressHandler{ocr, progressTracker};
    auto ocrResult = ocr.engine->recognize(
        ocrImage,
        job.langIndices,
        job.ocrFeatures,
        &ocrProgressHandler);

    progressTracker.finish();

    LINK_LOCK(ocr.link);
    if (ocr.link.terminateJobs)
        return;

    ocr.link.results.push_back({std::move(ocrResult), job.timestamp});
}


static void threadLoop(DpsoOcr* ocr)
{
    while (true) {
        Job job;

        {
            LINK_LOCK(ocr->link);

            if (ocr->link.terminateThread)
                break;

            if (!ocr->link.jobQueue.empty()) {
                job = std::move(ocr->link.jobQueue.front());
                assert(job.screenshot);
                ocr->link.jobQueue.pop();

                ocr->link.jobActive = true;

                // Although processJob() will reset progress to zero,
                // this should also be done before incrementing
                // curJob so that we don't return the progress of the
                // previous job from dpsoOcrGetProgress() before
                // the new one starts.
                ocr->link.progress.curJobProgress = 0;

                ++ocr->link.progress.curJob;
            } else if (ocr->link.jobActive) {
                ocr->link.jobActive = false;
                ocr->link.progress = {};
            }
        }

        if (!job.screenshot) {
            std::this_thread::sleep_for(threadIdleTime);
            continue;
        }

        processJob(*ocr, job);
    }
}


bool dpsoOcrQueueJob(DpsoOcr* ocr, const DpsoOcrJobArgs* jobArgs)
{
    if (!backend) {
        dpsoSetError("Library is not initialized");
        return false;
    }

    if (!ocr) {
        dpsoSetError("ocr is null");
        return false;
    }

    if (!jobArgs) {
        dpsoSetError("jobArgs is null");
        return false;
    }

    if (dpsoRectIsEmpty(&jobArgs->screenRect)) {
        dpsoSetError("jobArgs->screenRect is empty");
        return false;
    }

    if (ocr->numActiveLangs == 0) {
        dpsoSetError("No active languages");
        return false;
    }

    START_TIMING(takeScreenshot);

    std::unique_ptr<dpso::backend::Screenshot> screenshot;
    try {
        screenshot = backend->takeScreenshot(
            dpso::Rect{jobArgs->screenRect});
    } catch (dpso::backend::ScreenshotError& e) {
        dpsoSetError("Can't take screenshot: %s", e.what());
        return false;
    }

    assert(screenshot);

    END_TIMING(
        takeScreenshot,
        "Take screenshot (%ix%i px)",
        screenshot->getWidth(),
        screenshot->getHeight());

    dpso::ocr::OcrFeatures ocrFeatures{};
    if (jobArgs->flags & dpsoOcrJobTextSegmentation)
        ocrFeatures |= dpso::ocr::ocrFeatureTextSegmentation;

    Job job{
        std::move(screenshot),
        getActiveLangIndices(*ocr),
        ocrFeatures,
        createTimestamp()
    };

    LINK_LOCK(ocr->link);

    ocr->link.jobQueue.push(std::move(job));
    ++ocr->link.progress.totalJobs;

    return true;
}


bool dpsoOcrProgressEqual(
    const DpsoOcrProgress* a, const DpsoOcrProgress* b)
{
    return
        a
        && b
        && a->curJobProgress == b->curJobProgress
        && a->curJob == b->curJob
        && a->totalJobs == b->totalJobs;
}


void dpsoOcrGetProgress(const DpsoOcr* ocr, DpsoOcrProgress* progress)
{
    if (!ocr || !progress)
        return;

    LINK_LOCK(ocr->link);
    *progress = ocr->link.progress;
}


bool dpsoOcrHasPendingJobs(const DpsoOcr* ocr)
{
    if (!ocr)
        return false;

    LINK_LOCK(ocr->link);
    return ocr->link.jobsPending();
}


void dpsoOcrFetchResults(DpsoOcr* ocr, DpsoOcrJobResults* results)
{
    if (!ocr || !results)
        return;

    {
        LINK_LOCK(ocr->link);

        ocr->fetchedResults.clear();
        ocr->fetchedResults.swap(ocr->link.results);
    }

    ocr->returnedResults.clear();
    ocr->returnedResults.reserve(ocr->fetchedResults.size());
    for (const auto& result : ocr->fetchedResults)
        ocr->returnedResults.push_back(
            {result.ocrResult.text.c_str(),
                result.ocrResult.text.size(),
                result.timestamp.c_str()});

    results->items = ocr->returnedResults.data();
    results->numItems = ocr->returnedResults.size();
}


static void waitJobsToFinish(const DpsoOcr& ocr)
{
    while (true) {
        {
            LINK_LOCK(ocr.link);
            if (!ocr.link.jobsPending())
                break;
        }

        std::this_thread::sleep_for(threadIdleTime);
    }
}


void dpsoOcrWaitJobsToComplete(
    DpsoOcr* ocr,
    DpsoOcrProgressCallback progressCallback,
    void* userData)
{
    if (!ocr)
        return;

    {
        LINK_LOCK(ocr->link);
        ocr->link.waitingForResults = true;
        ocr->link.waitingProgressCallback = progressCallback;
        ocr->link.waitingUserData = userData;
    }

    waitJobsToFinish(*ocr);

    LINK_LOCK(ocr->link);
    ocr->link.waitingForResults = false;

    if (ocr->link.terminateJobs) {
        // dpsoOcrTerminateJobs() was called from the progress
        // callback.
        ocr->link.results.clear();
        ocr->link.terminateJobs = false;
    }
}


void dpsoOcrTerminateJobs(DpsoOcr* ocr)
{
    if (!ocr)
        return;

    {
        LINK_LOCK(ocr->link);

        ocr->link.jobQueue = {};
        ocr->link.terminateJobs = true;

        if (ocr->link.waitingForResults)
            // dpsoOcrWaitJobsToComplete() will set terminateJobs to
            // false.
            return;
    }

    waitJobsToFinish(*ocr);

    LINK_LOCK(ocr->link);

    ocr->link.results.clear();
    ocr->link.terminateJobs = false;
}


namespace dpso::ocr {


void init(dpso::backend::Backend& backend)
{
    ::backend = &backend;
}


void shutdown()
{
    ::backend = nullptr;
}


}
