
#include "ocr_private.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <clocale>
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

#include "backend/backend.h"
#include "backend/screenshot.h"
#include "backend/screenshot_error.h"
#include "error.h"
#include "geometry.h"
#include "geometry_c.h"
#include "img.h"
#include "ocr_engine/ocr_engine.h"
#include "progress_tracker.h"
#include "timing.h"


// Tesseract versions before 4.1.0 only work with "C" locale. It's
// a really big pain in the ass since we run Tesseract in a separate
// thread. It's possible neither to call setlocale() from the
// background thread nor let the user to change the locale while OCR
// is active.
//
// When the locale is changed and restored is documented in
// dpsoQueueJob(). We don't restore the locale early on dpsoUpdate()
// call since there's no way to tell this to the user trough the API.
static std::string lastLocale;
static bool localeChanged;


static void setCLocale()
{
    if (localeChanged)
        return;

    if (auto* locale = std::setlocale(LC_ALL, nullptr)) {
        lastLocale = locale;
        std::setlocale(LC_ALL, "C");
        localeChanged = true;
    }
}


static void restoreLocale()
{
    if (!localeChanged)
        return;

    std::setlocale(LC_ALL, lastLocale.c_str());
    localeChanged = false;
}


static std::unique_ptr<dpso::OcrEngine> ocrEngine;


namespace {


struct Lang {
    int langIdx;
    bool isActive;
};


}


// The public API gives a sorted list of language codes, while in
// OcrEngine they may be in arbitrary order. A Lang at the language
// index from public API refers to the state of the engine's language
// at Lang::langIdx.
static std::vector<Lang> langs;
static int numActiveLangs;


static void cacheLangs()
{
    langs.clear();
    langs.reserve(ocrEngine->getNumLangs());

    for (int i = 0; i < ocrEngine->getNumLangs(); ++i)
        langs.push_back({i, false});

    std::sort(
        langs.begin(), langs.end(),
        [](const Lang& a, const Lang& b)
        {
            return std::strcmp(
                ocrEngine->getLangCode(a.langIdx),
                ocrEngine->getLangCode(b.langIdx)) < 0;
        });
}


int dpsoGetNumLangs(void)
{
    return langs.size();
}


const char* dpsoGetLangCode(int langIdx)
{
    if (langIdx < 0
            || static_cast<std::size_t>(langIdx) >= langs.size())
        return "";

    return ocrEngine->getLangCode(langs[langIdx].langIdx);
}


const char* dpsoGetDefaultLangCode(void)
{
    return ocrEngine->getDefaultLangCode();
}


const char* dpsoGetLangName(const char* langCode)
{
    return ocrEngine->getLangName(langCode);
}


int dpsoGetLangIdx(const char* langCode)
{
    const auto iter = std::lower_bound(
        langs.begin(), langs.end(), langCode,
        [](const Lang& lang, const char* langCode)
        {
            return std::strcmp(
                ocrEngine->getLangCode(lang.langIdx), langCode) < 0;
        });

    if (iter != langs.end()
            && std::strcmp(
                ocrEngine->getLangCode(iter->langIdx), langCode) == 0)
        return iter - langs.begin();

    return -1;
}


int dpsoGetLangIsActive(int langIdx)
{
    if (langIdx < 0
            || static_cast<std::size_t>(langIdx) >= langs.size())
        return false;

    return langs[langIdx].isActive;
}


void dpsoSetLangIsActive(int langIdx, int newIsActive)
{
    if (langIdx < 0
            || static_cast<std::size_t>(langIdx) >= langs.size()
            || newIsActive == langs[langIdx].isActive)
        return;

    langs[langIdx].isActive = newIsActive;

    if (newIsActive)
        ++numActiveLangs;
    else
        --numActiveLangs;
}


int dpsoGetNumActiveLangs()
{
    return numActiveLangs;
}


static std::vector<int> getActiveLangIndices()
{
    std::vector<int> result;
    result.reserve(numActiveLangs);

    for (const auto& lang : langs)
        if (lang.isActive)
            result.push_back(lang.langIdx);

    return result;
}


namespace {


// 19 characters for ISO 8601 (YYYY-MM-DD HH:MM:SS) + null.
using Timestamp = std::array<char, 20>;


struct Job {
    std::unique_ptr<dpso::backend::Screenshot> screenshot;
    std::vector<int> langIndices;
    Timestamp timestamp;
    unsigned flags;
};


struct JobResult {
    dpso::OcrResult ocrResult;
    Timestamp timestamp;
};


}


static Timestamp createTimestamp()
{
    // We are still targeting old GCC versions, so double braces are
    // needed to avoid -Wmissing-field-initializers.
    Timestamp timestamp{{}};

    const auto time = std::time(nullptr);
    if (const auto* tm = std::localtime(&time))
        if (std::strftime(
                timestamp.data(), timestamp.size(),
                "%Y-%m-%d %H:%M:%S", tm) == 0)
            timestamp[0] = 0;

    return timestamp;
}


// Link between main and background threads.
static struct {
    std::mutex lock;

    std::queue<Job> jobQueue;
    bool jobActive;

    bool waitingForResults;
    DpsoProgressCallback waitingProgressCallback;
    void* waitingUserData;

    DpsoProgress progress;
    bool progressIsNew;

    std::vector<JobResult> results;

    bool terminateJobs;
    bool terminateThread;

    void reset()
    {
        clearJobQueue();
        jobActive = false;

        waitingForResults = false;

        progress = {};
        progressIsNew = false;

        results.clear();

        terminateJobs = false;
        terminateThread = false;
    }

    void clearJobQueue()
    {
        // We don't use = {} with std::queue for compatibility
        // with older GCC versions where explicit default
        // constructors are not fixed. See P0935R0.
        jobQueue = std::queue<Job>();
    }

    bool jobsPending() const
    {
        return !jobQueue.empty() || jobActive;
    }
} link;


#define LINK_LOCK std::lock_guard<std::mutex> guard(link.lock)


static dpso::OcrImage prepareScreenshot(
    const dpso::backend::Screenshot& screenshot,
    dpso::ProgressTracker& progressTracker)
{
    static std::vector<std::uint8_t> buffers[3];

    const int imageScale = 4;
    const auto bufferW = screenshot.getWidth() * imageScale;
    const auto bufferH = screenshot.getHeight() * imageScale;
    const auto bufferPitch = bufferW;

    for (auto& buffer : buffers)
        buffer.resize(bufferH * bufferPitch);

    START_TIMING(screenshotGetData);
    screenshot.getGrayscaleData(&buffers[0][0], bufferPitch);
    END_TIMING(
        screenshotGetData,
        "screenshot.getGrayscaleData (%ix%i px)",
        screenshot.getWidth(), screenshot.getHeight());

    START_TIMING(imageResizing);
    dpso::img::resize(
        &buffers[0][0],
        screenshot.getWidth(), screenshot.getHeight(), bufferPitch,
        &buffers[1][0],
        bufferW, bufferH, bufferPitch,
        &progressTracker);
    END_TIMING(
        imageResizing,
        "Image resizing (%ix%i px -> %ix%i px, x%i)",
        screenshot.getWidth(), screenshot.getHeight(),
        bufferW, bufferH,
        imageScale);

    const int unsharpMaskRadius = 10;

    START_TIMING(unsharpMasking);
    dpso::img::unsharpMask(
        &buffers[1][0], bufferPitch,
        &buffers[0][0], bufferPitch,
        &buffers[2][0], bufferPitch,
        bufferW, bufferH,
        unsharpMaskRadius,
        1.0f,
        &progressTracker);
    END_TIMING(
        unsharpMasking,
        "Unsharp masking (radius=%i, %ix%i px)",
        unsharpMaskRadius, bufferW, bufferH);

    return {&buffers[0][0], bufferW, bufferH, bufferPitch};
}


static bool ocrProgressCallback(int progress, void* userData)
{
    auto* progressTracker = (
        static_cast<dpso::ProgressTracker*>(userData));
    assert(progressTracker);
    progressTracker->update(progress / 100.0f);

    bool waitingForResults;
    {
        LINK_LOCK;
        waitingForResults = link.waitingForResults;
    }

    if (waitingForResults && link.waitingProgressCallback)
        link.waitingProgressCallback(link.waitingUserData);

    LINK_LOCK;
    return !link.terminateJobs;
}


static void progressTrackerFn(float progress, void* userData)
{
    (void)userData;

    LINK_LOCK;
    link.progress.curJobProgress = progress * 100;
    link.progressIsNew = true;
}


static bool dumpDebugImage;


static void processJob(const Job& job)
{
    assert(job.screenshot);
    assert(!job.langIndices.empty());

    // There are 3 progress jobs: resizing and unsharp masking in
    // prepareScreenshot() and OCR.
    dpso::ProgressTracker progressTracker(3, progressTrackerFn);
    progressTracker.start();

    const auto ocrImage = prepareScreenshot(
        *job.screenshot, progressTracker);

    if (dumpDebugImage)
        dpso::img::savePgm(
            "dpso_debug.pgm",
            ocrImage.data,
            ocrImage.width, ocrImage.height, ocrImage.pitch);

    progressTracker.advanceJob();

    dpso::OcrFeatures ocrFeatures = 0;
    if (job.flags & dpsoJobTextSegmentation)
        ocrFeatures |= dpso::ocrFeatureTextSegmentation;

    auto ocrResult = ocrEngine->recognize(
        ocrImage, job.langIndices, ocrFeatures,
        ocrProgressCallback, &progressTracker);

    progressTracker.finish();

    LINK_LOCK;
    if (link.terminateJobs)
        return;

    link.results.push_back({std::move(ocrResult), job.timestamp});
}


static void threadLoop()
{
    while (true) {
        Job job;

        {
            LINK_LOCK;

            if (link.terminateThread)
                break;

            if (!link.jobQueue.empty()) {
                job = std::move(link.jobQueue.front());
                assert(job.screenshot);
                link.jobQueue.pop();

                link.jobActive = true;

                // Although processJob() will reset result to zero,
                // this should also be done before incrementing
                // curJob so we don't return the progress of the
                // previous job from dpsoGetProgress() before
                // the new one starts.
                link.progress.curJobProgress = 0;

                ++link.progress.curJob;
                link.progressIsNew = true;
            } else if (link.jobActive) {
                link.jobActive = false;

                link.progress = {};
                link.progressIsNew = true;
            }
        }

        if (!job.screenshot) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(1000 / 60));
            continue;
        }

        processJob(job);
    }
}


int dpsoQueueJob(const struct DpsoJobArgs* jobArgs)
{
    if (!jobArgs) {
        dpsoSetError("jobArgs is null");
        return false;
    }

    if (dpsoRectIsEmpty(&jobArgs->screenRect)) {
        dpsoSetError("jobArgs->screenRect is empty");
        return false;
    }

    if (numActiveLangs == 0) {
        dpsoSetError("No active languages");
        return false;
    }

    START_TIMING(takeScreenshot);

    std::unique_ptr<dpso::backend::Screenshot> screenshot;
    try {
        screenshot = dpso::backend::getBackend().takeScreenshot(
            dpso::Rect{jobArgs->screenRect});
    } catch (dpso::backend::ScreenshotError& e) {
        dpsoSetError((
            std::string{"Can't take screenshot: "} + e.what()
            ).c_str());
        return false;
    }

    assert(screenshot);

    END_TIMING(
        takeScreenshot,
        "Take screenshot (%ix%i px)",
        screenshot->getWidth(),
        screenshot->getHeight());

    Job job{
        std::move(screenshot),
        getActiveLangIndices(),
        createTimestamp(),
        jobArgs->flags
    };

    setCLocale();

    LINK_LOCK;

    link.jobQueue.push(std::move(job));

    ++link.progress.totalJobs;
    link.progressIsNew = true;

    return true;
}


void dpsoGetProgress(struct DpsoProgress* progress, int* isNew)
{
    LINK_LOCK;

    if (progress)
        *progress = link.progress;

    if (isNew)
        *isNew = link.progressIsNew;

    link.progressIsNew = false;
}


int dpsoGetJobsPending(void)
{
    LINK_LOCK;
    return link.jobsPending();
}


static std::vector<JobResult> fetchedResults;
static std::vector<DpsoJobResult> returnResults;


int dpsoFetchResults(DpsoResultFetchingMode fetchingMode)
{
    LINK_LOCK;

    if (link.results.empty()
            || (fetchingMode == dpsoFetchFullChain
                && link.jobsPending()))
        return false;

    fetchedResults.clear();
    fetchedResults.swap(link.results);

    returnResults.clear();
    returnResults.reserve(fetchedResults.size());
    for (const auto& result : fetchedResults)
        returnResults.push_back(
            {result.ocrResult.getText(),
                result.ocrResult.getTextLen(),
                result.timestamp.data()});

    if (!link.jobsPending())
        restoreLocale();

    return true;
}


void dpsoGetFetchedResults(struct DpsoJobResults* results)
{
    if (!results)
        return;

    results->items = returnResults.data();
    results->numItems = returnResults.size();
}


static void waitJobsToFinish()
{
    while (true) {
        {
            LINK_LOCK;
            if (!link.jobsPending())
                break;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(10));
    }
}


void dpsoWaitForResults(
    DpsoProgressCallback progressCallback,
    void* userData)
{
    {
        LINK_LOCK;
        link.waitingForResults = true;
        link.waitingProgressCallback = progressCallback;
        link.waitingUserData = userData;
    }

    waitJobsToFinish();

    LINK_LOCK;
    link.waitingForResults = false;

    if (link.terminateJobs) {
        // dpsoTerminateJobs() was called from the status callback
        // during waiting.
        link.results.clear();

        link.terminateJobs = false;
    }

    restoreLocale();
}


void dpsoTerminateJobs(void)
{
    {
        LINK_LOCK;

        link.clearJobQueue();
        link.terminateJobs = true;

        if (link.waitingForResults)
            // dpsoWaitForResults() will set terminateJobs to false.
            return;
    }

    waitJobsToFinish();

    // Locking here is not actually necessary since the idling
    // background thread doesn't access variables we use below.
    LINK_LOCK;

    link.results.clear();

    link.terminateJobs = false;

    restoreLocale();
}


namespace dpso {
namespace ocr {


static std::thread bgThread;


void init()
{
    setCLocale();
    ocrEngine = OcrEngine::create();
    restoreLocale();

    assert(langs.empty());
    assert(numActiveLangs == 0);
    cacheLangs();

    assert(!link.jobsPending());
    link.reset();

    const char* dumpDebugImageEnvVar = std::getenv(
        "DPSO_DUMP_DEBUG_IMAGE");
    dumpDebugImage = (dumpDebugImageEnvVar
        && std::strcmp(dumpDebugImageEnvVar, "0") != 0);

    bgThread = std::thread(threadLoop);

    fetchedResults.clear();
    returnResults.clear();
}


void shutdown()
{
    langs.clear();
    numActiveLangs = 0;

    dpsoTerminateJobs();

    {
        LINK_LOCK;
        link.terminateThread = true;
    }
    bgThread.join();
    link.terminateThread = false;

    setCLocale();
    ocrEngine.reset();
    restoreLocale();
}


}
}
