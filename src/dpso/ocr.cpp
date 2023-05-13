
#include "ocr.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <functional>
#include <mutex>
#include <optional>
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
#include "backend/screenshot.h"
#include "backend/screenshot_error.h"
#include "geometry.h"
#include "geometry_c.h"
#include "img.h"
#include "ocr/engine.h"
#include "ocr/recognizer.h"
#include "ocr/recognizer_error.h"
#include "ocr_registry.h"


// A note on parallelism
//
// Since we don't require any thread safety guarantees from
// ocr::Recognizer, we never use it from multiple threads. Only
// ocr::Recognizer::recognize() is called after DpsoOcr is created;
// everything else is cached. An alternative would be to protect
// ocr::Recognizer access with a mutex, but it's impractical for
// functions like dpsoOcrGetLangCode() to block during OCR since it
// takes significant time.


static dpso::backend::Backend* backend;


const std::chrono::milliseconds threadIdleTime{10};


namespace {


struct Lang {
    std::string code;
    std::string name;
    // The public API gives languages sorted by codes, while in
    // ocr::Engine they may be in arbitrary order. A Lang at the index
    // from the public API refers to the ocr::Engine language at
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
    mutable std::mutex mutex;

    std::queue<Job> jobQueue;
    bool jobActive;

    DpsoOcrProgress progress;

    std::vector<JobResult> results;

    bool terminateJobs;
    bool terminateThread;

    bool jobsPending() const
    {
        return !jobQueue.empty() || jobActive;
    }
};


#define LINK_LOCK(L) const std::lock_guard linkLockGuard{L.mutex}


}


struct DpsoOcr {
    std::unique_ptr<dpso::ocr::Recognizer> recognizer;

    std::shared_ptr<dpso::ocr::OcrRegistry> registry;

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


static void reloadLangs(DpsoOcr& ocr)
{
    std::vector<std::string> prevActiveLangCodes;
    prevActiveLangCodes.reserve(ocr.numActiveLangs);
    for (const auto& lang : ocr.langs)
        if (lang.isActive)
            prevActiveLangCodes.push_back(lang.code);

    ocr.numActiveLangs = 0;

    ocr.langs.clear();
    ocr.langs.reserve(ocr.recognizer->getNumLangs());

    for (int i = 0; i < ocr.recognizer->getNumLangs(); ++i)
        ocr.langs.push_back(
            {
                ocr.recognizer->getLangCode(i),
                ocr.recognizer->getLangName(i),
                i,
                false
            });

    std::sort(
        ocr.langs.begin(), ocr.langs.end(),
        [](const Lang& a, const Lang& b)
        {
            return a.code < b.code;
        });

    for (const auto& langCode : prevActiveLangCodes)
        dpsoOcrSetLangIsActive(
            &ocr, dpsoOcrGetLangIdx(&ocr, langCode.c_str()), true);
}


static void threadLoop(DpsoOcr& ocr);


DpsoOcr* dpsoOcrCreate(int engineIdx, const char* dataDir)
{
    if (engineIdx < 0
            || static_cast<std::size_t>(engineIdx)
                >= dpso::ocr::Engine::getCount()) {
        dpsoSetError("engineIdx is out of bounds");
        return nullptr;
    }

    const auto& ocrEngine = dpso::ocr::Engine::get(engineIdx);

    // We don't use OcrUPtr here because dpsoOcrDelete() expects
    // a joinable thread.
    auto ocr = std::make_unique<DpsoOcr>();

    try {
        ocr->recognizer = ocrEngine.createRecognizer(dataDir);
    } catch (dpso::ocr::RecognizerError& e) {
        dpsoSetError("Can't create recognizer: %s", e.what());
        return nullptr;
    }

    ocr->registry = dpso::ocr::OcrRegistry::get(
        ocrEngine.getInfo().id.c_str(), dataDir);
    ocr->registry->add(*ocr);

    ocr->defaultLangCode = ocr->recognizer->getDefaultLangCode();
    reloadLangs(*ocr);

    ocr->thread = std::thread(threadLoop, std::ref(*ocr));

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

    ocr->registry->remove(*ocr);

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
            || static_cast<std::size_t>(langIdx) >= ocr->langs.size())
        return "";

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

    auto ocrResult = ocr.recognizer->recognize(
        ocrImage,
        job.langIndices,
        job.ocrFeatures,
        [&](int progress)
        {
            progressTracker.update(progress / 100.0f);

            LINK_LOCK(ocr.link);
            return !ocr.link.terminateJobs;
        });

    progressTracker.finish();

    LINK_LOCK(ocr.link);
    if (ocr.link.terminateJobs)
        return;

    ocr.link.results.push_back({std::move(ocrResult), job.timestamp});
}


static void threadLoop(DpsoOcr& ocr)
{
    while (true) {
        std::optional<Job> job;

        {
            LINK_LOCK(ocr.link);

            if (ocr.link.terminateThread)
                break;

            if (!ocr.link.jobQueue.empty()) {
                job = std::move(ocr.link.jobQueue.front());
                ocr.link.jobQueue.pop();

                ocr.link.jobActive = true;

                // Although processJob() will reset progress to zero,
                // this should also be done before incrementing
                // curJob so that we don't return the progress of the
                // previous job from dpsoOcrGetProgress() before
                // the new one starts.
                ocr.link.progress.curJobProgress = 0;

                ++ocr.link.progress.curJob;
            } else if (ocr.link.jobActive) {
                ocr.link.jobActive = false;
                ocr.link.progress = {};
            }
        }

        if (job)
            processJob(ocr, *job);
        else
            std::this_thread::sleep_for(threadIdleTime);
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

    if (ocr->registry->getLangManagerIsActive()) {
        dpsoSetError("Language manager is active");
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

    *results = {
        ocr->returnedResults.data(),
        static_cast<int>(ocr->returnedResults.size())
    };
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


void dpsoOcrTerminateJobs(DpsoOcr* ocr)
{
    if (!ocr)
        return;

    {
        LINK_LOCK(ocr->link);

        ocr->link.jobQueue = {};
        ocr->link.terminateJobs = true;
    }

    waitJobsToFinish(*ocr);

    LINK_LOCK(ocr->link);

    ocr->link.results.clear();
    ocr->link.terminateJobs = false;
}


namespace dpso::ocr {


void onLangManagerCreated(DpsoOcr& ocr)
{
    waitJobsToFinish(ocr);
}


void onLangManagerDeleted(DpsoOcr& ocr)
{
    ocr.recognizer->reloadLangs();
    reloadLangs(ocr);
}


void init(dpso::backend::Backend& backend)
{
    ::backend = &backend;
}


void shutdown()
{
    ::backend = nullptr;
}


}
