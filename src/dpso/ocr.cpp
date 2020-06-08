
#include "ocr_private.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <clocale>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "backend/backend.h"
#include "img.h"
#include "ocr_engine/ocr_engine.h"
#include "progress_tracker.h"
#include "str.h"
#include "timing.h"


// Tesseract only works with "C" locale. Since version 4.0 it's even
// constrained in the TessBaseAPI constructor. It's a really big pain
// in the ass, since we run Tesseract in a separate thread. It's
// possible neither to call setlocale() from the background thread
// nor let the user to change the locale while OCR is active.
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
// OcrEngine they ma be in arbitrary order. A Lang at the language
// index from public API refers to the state of the engine's language
// at Lang::langIdx.
static std::vector<Lang> langs;
static std::vector<int> activeLangIndices;


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


int dpsoGetLangIdx(const char* langCode, size_t langCodeLen)
{
    const auto iter = std::lower_bound(
        langs.begin(), langs.end(), langCode,
        [langCodeLen](const Lang& lang, const char* langCode)
        {
            return dpso::str::cmpSubStr(
                ocrEngine->getLangCode(lang.langIdx),
                langCode,
                langCodeLen) < 0;
        });

    if (iter != langs.end()
            && dpso::str::cmpSubStr(
                ocrEngine->getLangCode(iter->langIdx),
                langCode,
                langCodeLen) == 0)
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

    auto findActiveLang = [](int langIdx)
    {
        return std::find(
            activeLangIndices.begin(),
            activeLangIndices.end(),
            langs[langIdx].langIdx);
    };

    if (newIsActive) {
        assert(findActiveLang(langIdx) == activeLangIndices.end());
        activeLangIndices.push_back(langs[langIdx].langIdx);
    } else {
        const auto activeLangIter = findActiveLang(langIdx);
        assert(activeLangIter != activeLangIndices.end());

        activeLangIndices.erase(activeLangIter);
    }
}


int dpsoGetNumActiveLangs()
{
    return activeLangIndices.size();
}


namespace {


// 19 characters for ISO 8601 (YYYY-MM-DD HH:MM:SS) + null.
using Timestamp = std::array<char, 20>;


struct Job {
    std::unique_ptr<dpso::backend::Screenshot> screenshot;
    std::vector<int> langIndices;
    Timestamp timestamp;
    int flags;
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
    Timestamp timestamp {{}};

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


static struct {
    std::vector<std::uint8_t> buffers[3];
    int w;
    int h;
    int pitch;

    void resize(int newW, int newH)
    {
        assert(newW >= 0);
        assert(newH >= 0);

        w = newW;
        h = newH;
        pitch = w;

        for (auto& buf : buffers)
            buf.resize(h * pitch);
    }
} imageBuffers;


// The final image will be in imageBuffers.buffers[0].
static void prepareScreenshot(
    dpso::backend::Screenshot& screenshot,
    dpso::ProgressTracker& progressTracker)
{
    const int imageScale = 4;

    imageBuffers.resize(
        screenshot.getWidth() * imageScale,
        screenshot.getHeight() * imageScale);

    START_TIMING(screenshotGetData);
    screenshot.getGrayscaleData(
        &imageBuffers.buffers[0][0], imageBuffers.pitch);
    END_TIMING(
        screenshotGetData,
        "screenshot.getGrayscaleData (%ix%i px)",
        screenshot.getWidth(), screenshot.getHeight());

    START_TIMING(imageResizing);
    dpso::img::resize(
        &imageBuffers.buffers[0][0],
        screenshot.getWidth(), screenshot.getHeight(),
        imageBuffers.pitch,
        &imageBuffers.buffers[1][0], imageBuffers.w, imageBuffers.h,
        imageBuffers.pitch,
        &progressTracker);
    END_TIMING(
        imageResizing,
        "Image resizing (%ix%i px -> %ix%i px, x%i)",
        screenshot.getWidth(), screenshot.getHeight(),
        imageBuffers.w, imageBuffers.h,
        imageScale);

    const int unsharpMaskRadius = 10;

    START_TIMING(unsharpMasking);
    dpso::img::unsharpMask(
        &imageBuffers.buffers[1][0], imageBuffers.pitch,
        &imageBuffers.buffers[0][0], imageBuffers.pitch,
        &imageBuffers.buffers[2][0], imageBuffers.pitch,
        imageBuffers.w, imageBuffers.h,
        unsharpMaskRadius,
        1.0f,
        &progressTracker);
    END_TIMING(
        unsharpMasking,
        "Unsharp masking (radius=%i, %ix%i px)",
        unsharpMaskRadius,
        imageBuffers.w, imageBuffers.h);
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

    {
        LINK_LOCK;
        return !link.terminateJobs;
    }
}


static void progressTrackerFn(float progress, void* userData)
{
    (void)userData;

    LINK_LOCK;
    link.progress.curJobProgress = progress * 100;
    link.progressIsNew = true;
}


static void processJob(const Job& job)
{
    assert(job.screenshot);
    assert(!job.langIndices.empty());

    // There are 3 progress jobs: resizing and unsharp masking in
    // prepareScreenshot() and OCR.
    dpso::ProgressTracker progressTracker(3, progressTrackerFn);
    progressTracker.start();

    prepareScreenshot(*job.screenshot, progressTracker);

    if (job.flags & dpsoJobDumpDebugImage)
        dpso::img::savePgm(
            "dpso_debug.pgm",
            &imageBuffers.buffers[0][0],
            imageBuffers.w, imageBuffers.h, imageBuffers.pitch);

    progressTracker.advanceJob();

    const dpso::OcrImage ocrImage {
        &imageBuffers.buffers[0][0],
        imageBuffers.w, imageBuffers.h,
        imageBuffers.pitch
    };

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
    if (!jobArgs)
        return false;

    if (activeLangIndices.empty())
        return false;

    START_TIMING(takeScreenshot);
    auto screenshot = dpso::backend::getBackend().takeScreenshot(
        dpso::Rect{jobArgs->screenRect});
    END_TIMING(
        takeScreenshot,
        "Take screenshot (%ix%i px)",
        screenshot ? screenshot->getWidth() : -1,
        screenshot ? screenshot->getHeight() : -1);

    if (!screenshot)
        return false;

    static const int minScreenshotSize = 5;
    if (screenshot->getWidth() < minScreenshotSize
            || screenshot->getHeight() < minScreenshotSize)
        return false;

    Job job {
        std::move(screenshot),
        activeLangIndices,
        createTimestamp(),
        jobArgs->flags
    };

    setCLocale();

    {
        LINK_LOCK;

        link.jobQueue.push(std::move(job));

        ++link.progress.totalJobs;
        link.progressIsNew = true;
    }

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

    // vector::data() is not guaranteed to be null for an empty vector
    if (returnResults.empty())
        results->items = nullptr;
    else
        results->items = &returnResults[0];

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
    assert(activeLangIndices.empty());
    cacheLangs();

    assert(!link.jobsPending());
    link.reset();

    bgThread = std::thread(threadLoop);

    fetchedResults.clear();
    returnResults.clear();
}


void shutdown()
{
    langs.clear();
    activeLangIndices.clear();

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
