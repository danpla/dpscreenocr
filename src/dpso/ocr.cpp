
#include "ocr.h"

#include <algorithm>
#include <cassert>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <functional>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "dpso_utils/error_set.h"
#include "dpso_utils/geometry.h"
#include "dpso_utils/progress_tracker.h"
#include "dpso_utils/strftime.h"
#include "dpso_utils/synchronized.h"
#include "dpso_utils/timing.h"

#include "backend/backend.h"
#include "backend/screenshot.h"
#include "backend/screenshot_error.h"
#include "img.h"
#include "ocr/engine.h"
#include "ocr/recognizer.h"
#include "ocr/recognizer_error.h"
#include "ocr_data_lock.h"


static dpso::backend::Backend* backend;


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
    std::condition_variable threadActionCondVar;
    std::condition_variable jobsDoneCondVar;

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


}


struct DpsoOcr {
    dpso::ocr::DataLockObserver dataLockObserver;

    std::unique_ptr<dpso::ocr::Recognizer> recognizer;

    std::string defaultLangCode;
    std::vector<Lang> langs;
    int numActiveLangs;

    dpso::Synchronized<Link> link;
    std::thread thread;

    std::vector<std::uint8_t> imgBuffers[3];
    bool dumpDebugImages;

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
                false});

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


static void waitJobsToFinish(DpsoOcr& ocr)
{
    auto link = ocr.link.getLock();
    link.wait(
        link->jobsDoneCondVar, [&]{ return !link->jobsPending(); });
}


static void threadLoop(DpsoOcr& ocr);


DpsoOcr* dpsoOcrCreate(int engineIdx, const char* dataDir)
{
    if (engineIdx < 0
            || static_cast<std::size_t>(engineIdx)
                >= dpso::ocr::Engine::getCount()) {
        dpso::setError("engineIdx is out of bounds");
        return nullptr;
    }

    const auto& ocrEngine = dpso::ocr::Engine::get(engineIdx);

    // We don't use OcrUPtr here because dpsoOcrDelete() expects
    // a joinable thread.
    auto ocr = std::make_unique<DpsoOcr>();

    ocr->dataLockObserver = dpso::ocr::DataLockObserver{
        ocrEngine.getInfo().id.c_str(),
        dataDir,
        [&ocr = *ocr]
        {
            waitJobsToFinish(ocr);
        },
        [&ocr = *ocr]
        {
            try {
                ocr.recognizer->reloadLangs();
            } catch (dpso::ocr::RecognizerError&) {
                return;
            }

            reloadLangs(ocr);
        }
    };

    if (ocr->dataLockObserver.getIsDataLocked()) {
        dpso::setError("OCR data is locked");
        return nullptr;
    }

    try {
        ocr->recognizer = ocrEngine.createRecognizer(dataDir);
    } catch (dpso::ocr::RecognizerError& e) {
        dpso::setError("Can't create recognizer: {}", e.what());
        return nullptr;
    }

    ocr->defaultLangCode = ocr->recognizer->getDefaultLangCode();
    reloadLangs(*ocr);

    ocr->thread = std::thread(threadLoop, std::ref(*ocr));

    const auto* dumpDebugImagesEnvVar = std::getenv(
        "DPSO_DUMP_DEBUG_IMAGES");
    ocr->dumpDebugImages =
        dumpDebugImagesEnvVar
        && *dumpDebugImagesEnvVar
        && std::strcmp(dumpDebugImagesEnvVar, "0") != 0;

    return ocr.release();
}


void dpsoOcrDelete(DpsoOcr* ocr)
{
    if (!ocr)
        return;

    dpsoOcrTerminateJobs(ocr);

    {
        auto link = ocr->link.getLock();
        link->terminateThread = true;
        link->threadActionCondVar.notify_one();
    }
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
    const dpso::backend::Screenshot& screenshot,
    std::vector<std::uint8_t> (&imgBuffers)[3],
    dpso::ProgressTracker& progressTracker,
    bool dumpDebugImages)
{
    const auto scale = 4;
    const auto bufferW = screenshot.getWidth() * scale;
    const auto bufferH = screenshot.getHeight() * scale;
    const auto bufferPitch = bufferW;

    for (auto& buffer : imgBuffers)
        buffer.resize(bufferH * bufferPitch);

    DPSO_START_TIMING(screenshotGetData);
    screenshot.getGrayscaleData(
        imgBuffers[0].data(), bufferPitch);
    DPSO_END_TIMING(
        screenshotGetData,
        "screenshot.getGrayscaleData ({}x{} px)",
        screenshot.getWidth(), screenshot.getHeight());

    if (dumpDebugImages)
        dpso::img::savePgm(
            "dpso_debug_1_original.pgm",
            imgBuffers[0].data(),
            screenshot.getWidth(),
            screenshot.getHeight(),
            bufferPitch);

    DPSO_START_TIMING(imageResizing);
    dpso::img::resize(
        imgBuffers[0].data(),
        screenshot.getWidth(), screenshot.getHeight(), bufferPitch,
        imgBuffers[1].data(),
        bufferW, bufferH, bufferPitch);
    DPSO_END_TIMING(
        imageResizing,
        "Image resizing ({}x{} px -> {}x{} px, x{})",
        screenshot.getWidth(), screenshot.getHeight(),
        bufferW, bufferH,
        scale);

    if (dumpDebugImages)
        dpso::img::savePgm(
            "dpso_debug_2_resize.pgm",
            imgBuffers[1].data(), bufferW, bufferH, bufferPitch);

    const auto unsharpMaskRadius = 10;
    const auto unsharpMaskAmount = 1.0f;

    dpso::ProgressTracker localProgressTracker(1, &progressTracker);
    localProgressTracker.advanceJob();

    DPSO_START_TIMING(unsharpMasking);
    dpso::img::unsharpMask(
        imgBuffers[1].data(), bufferPitch,
        imgBuffers[0].data(), bufferPitch,
        imgBuffers[2].data(), bufferPitch,
        bufferW, bufferH,
        unsharpMaskRadius,
        unsharpMaskAmount,
        &localProgressTracker);
    DPSO_END_TIMING(
        unsharpMasking,
        "Unsharp masking (radius={}, amount={}, {}x{} px)",
        unsharpMaskRadius, unsharpMaskAmount, bufferW, bufferH);

    localProgressTracker.finish();

    if (dumpDebugImages)
        dpso::img::savePgm(
            "dpso_debug_3_unsharp_mask.pgm",
            imgBuffers[0].data(), bufferW, bufferH, bufferPitch);

    return {imgBuffers[0].data(), bufferW, bufferH, bufferPitch};
}


static void processJob(DpsoOcr& ocr, const Job& job)
{
    dpso::ProgressTracker progressTracker{
        2,
        [&](float progress)
        {
            ocr.link.getLock()->progress.curJobProgress =
                progress * 100;
        }};

    progressTracker.advanceJob();
    const auto ocrImage = prepareScreenshot(
        *job.screenshot,
        ocr.imgBuffers,
        progressTracker,
        ocr.dumpDebugImages);

    progressTracker.advanceJob();

    auto ocrResult = ocr.recognizer->recognize(
        ocrImage,
        job.langIndices,
        job.ocrFeatures,
        [&](int progress)
        {
            progressTracker.update(progress / 100.0f);
            return !ocr.link.getLock()->terminateJobs;
        });

    progressTracker.finish();

    ocr.link.getLock()->results.push_back(
        {std::move(ocrResult), job.timestamp});
}


static void threadLoop(DpsoOcr& ocr)
{
    while (true) {
        Job job;

        {
            auto link = ocr.link.getLock();
            link.wait(
                link->threadActionCondVar,
                [&]
                {
                    return
                        link->terminateThread
                        || !link->jobQueue.empty();
                });

            if (link->terminateThread)
                break;

            job = std::move(link->jobQueue.front());
            link->jobQueue.pop();

            link->jobActive = true;

            // Although processJob() will reset progress to zero, this
            // should also be done before incrementing curJob so that
            // we don't return the progress of the previous job from
            // dpsoOcrGetProgress() before the new one starts.
            link->progress.curJobProgress = 0;
            ++link->progress.curJob;
        }

        processJob(ocr, job);

        auto link = ocr.link.getLock();

        link->jobActive = false;
        if (link->jobQueue.empty()) {
            link->progress = {};
            link->jobsDoneCondVar.notify_one();
        }
    }
}


bool dpsoOcrQueueJob(DpsoOcr* ocr, const DpsoOcrJobArgs* jobArgs)
{
    if (!backend) {
        dpso::setError("Library is not initialized");
        return false;
    }

    if (!ocr) {
        dpso::setError("ocr is null");
        return false;
    }

    if (!jobArgs) {
        dpso::setError("jobArgs is null");
        return false;
    }

    if (dpsoRectIsEmpty(&jobArgs->screenRect)) {
        dpso::setError("jobArgs->screenRect is empty");
        return false;
    }

    if (ocr->numActiveLangs == 0) {
        dpso::setError("No active languages");
        return false;
    }

    if (ocr->dataLockObserver.getIsDataLocked()) {
        dpso::setError("OCR data is locked");
        return false;
    }

    DPSO_START_TIMING(takeScreenshot);

    std::unique_ptr<dpso::backend::Screenshot> screenshot;
    try {
        screenshot = backend->takeScreenshot(
            dpso::Rect{jobArgs->screenRect});
    } catch (dpso::backend::ScreenshotError& e) {
        dpso::setError("Can't take screenshot: {}", e.what());
        return false;
    }

    DPSO_END_TIMING(
        takeScreenshot,
        "Take screenshot ({}x{} px)",
        screenshot->getWidth(),
        screenshot->getHeight());

    dpso::ocr::OcrFeatures ocrFeatures{};
    if (jobArgs->flags & dpsoOcrJobTextSegmentation)
        ocrFeatures |= dpso::ocr::ocrFeatureTextSegmentation;

    Job job{
        std::move(screenshot),
        getActiveLangIndices(*ocr),
        ocrFeatures,
        createTimestamp()};

    auto link = ocr->link.getLock();

    link->jobQueue.push(std::move(job));
    ++link->progress.totalJobs;
    link->threadActionCondVar.notify_one();

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
    if (ocr && progress)
        *progress = ocr->link.getLock()->progress;
}


bool dpsoOcrHasPendingJobs(const DpsoOcr* ocr)
{
    return ocr && ocr->link.getLock()->jobsPending();
}


void dpsoOcrFetchResults(DpsoOcr* ocr, DpsoOcrJobResults* results)
{
    if (!ocr || !results)
        return;

    ocr->fetchedResults.clear();
    ocr->fetchedResults.swap(ocr->link.getLock()->results);

    ocr->returnedResults.clear();
    ocr->returnedResults.reserve(ocr->fetchedResults.size());

    for (const auto& result : ocr->fetchedResults)
        ocr->returnedResults.push_back(
            {
                result.ocrResult.text.c_str(),
                result.ocrResult.text.size(),
                result.timestamp.c_str()});

    *results = {
        ocr->returnedResults.data(),
        static_cast<int>(ocr->returnedResults.size())};
}


void dpsoOcrTerminateJobs(DpsoOcr* ocr)
{
    if (!ocr)
        return;

    {
        auto link = ocr->link.getLock();
        link->jobQueue = {};
        link->terminateJobs = true;
    }

    waitJobsToFinish(*ocr);

    auto link = ocr->link.getLock();
    link->results.clear();
    link->terminateJobs = false;
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
