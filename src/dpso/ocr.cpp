
#include "ocr_private.h"

#include <cassert>
#include <chrono>
#include <clocale>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "tesseract/baseapi.h"
#include "tesseract/genericvector.h"
#include "tesseract/ocrclass.h"
#include "tesseract/strngs.h"

#include "backend/backend.h"
#include "img.h"
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

static tesseract::TessBaseAPI* tess;


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


namespace {


struct Lang {
    std::string code;
    bool active;
};


}


static std::vector<Lang> langs;
static int numActiveLangs;


static int cmpTessStrings(const void *a, const void *b)
{
    const auto* strA = static_cast<const STRING*>(a);
    const auto* strB = static_cast<const STRING*>(b);
    return std::strcmp(strA->c_str(), strB->c_str());
}


static void cacheLangs()
{
    langs.clear();

    setCLocale();

    tess->Init(nullptr, nullptr);

    GenericVector<STRING> langCodes;
    tess->GetAvailableLanguagesAsVector(&langCodes);
    langCodes.sort(cmpTessStrings);

    restoreLocale();

    for (int i = 0; i < langCodes.size(); ++i) {
        const auto& langCode = langCodes[i];
        if (langCode == "osd")
            continue;

        langs.push_back({langCode.c_str(), false});
    }
}


// String for TessBaseAPI::Init()
static std::string tessLangsStr;
static bool tessLangsStrValid;


static void updateTessLangsStr()
{
    if (tessLangsStrValid)
        return;

    tessLangsStr.clear();

    for (const auto& lang : langs) {
        if (!lang.active)
            continue;

        if (!tessLangsStr.empty())
            tessLangsStr += '+';
        tessLangsStr += lang.code;
    }

    tessLangsStrValid = true;
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

    return langs[langIdx].code.c_str();
}


int dpsoGetLangIsActive(int langIdx)
{
    if (langIdx < 0
            || static_cast<std::size_t>(langIdx) >= langs.size())
        return false;

    return langs[langIdx].active;
}


void dpsoSetLangIsActive(int langIdx, int newIsActive)
{
    if (langIdx < 0
            || static_cast<std::size_t>(langIdx) >= langs.size()
            || newIsActive == langs[langIdx].active)
        return;

    if (newIsActive)
        ++numActiveLangs;
    else
        --numActiveLangs;

    tessLangsStrValid = false;
    langs[langIdx].active = newIsActive;
}


int dpsoGetNumActiveLangs()
{
    return numActiveLangs;
}


// ISO 8601 needs 19 characters (YYYY-MM-DD HH:MM:SS) + null.
const std::size_t timestampBufSize = 20;


namespace {


struct Job {
    std::unique_ptr<dpso::backend::Screenshot> screenshot;
    std::string tessLangsStr;
    char timestamp[timestampBufSize];
    int flags;
};


struct JobResult {
    std::unique_ptr<char[]> text;
    std::size_t textLen;
    char timestamp[timestampBufSize];
};


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


namespace {


struct TessCancelData {
    ETEXT_DESC textDesc;
    dpso::ProgressTracker* progressTracker;
};


}


static bool tessCancelFunc(void* cancelData, int words)
{
    (void)words;

    bool waitingForResults;

    {
        const auto* tessCancelData = (
            static_cast<const TessCancelData*>(cancelData));

        tessCancelData->progressTracker->update(
            tessCancelData->textDesc.progress / 100.0f);

        std::lock_guard<std::mutex> guard(link.lock);
        waitingForResults = link.waitingForResults;
    }

    if (waitingForResults && link.waitingProgressCallback)
        link.waitingProgressCallback(link.waitingUserData);

    {
        std::lock_guard<std::mutex> guard(link.lock);
        return link.terminateJobs;
    }
}


static void progressTrackerFn(float progress, void* userData)
{
    (void)userData;

    std::lock_guard<std::mutex> guard(link.lock);
    link.progress.curJobProgress = progress * 100;
    link.progressIsNew = true;
}


static void processJob(const Job& job)
{
    assert(job.screenshot);
    assert(!job.tessLangsStr.empty());

    dpso::ProgressTracker progressTracker(3, progressTrackerFn);
    progressTracker.start();

    prepareScreenshot(*job.screenshot, progressTracker);

    if (job.flags & dpsoJobDumpDebugImage)
        dpso::img::savePgm(
            "dpso_debug.pgm",
            &imageBuffers.buffers[0][0],
            imageBuffers.w, imageBuffers.h, imageBuffers.pitch);

    progressTracker.advanceJob();

    // The locale is changed to "C" at this point.
    tess->Init(nullptr, job.tessLangsStr.c_str());

    tesseract::PageSegMode pageSegMode;
    if (job.flags & dpsoJobTextSegmentation)
        pageSegMode = tesseract::PSM_AUTO;
    else
        pageSegMode = tesseract::PSM_SINGLE_BLOCK;
    tess->SetPageSegMode(pageSegMode);

    tess->SetImage(
        &imageBuffers.buffers[0][0],
        imageBuffers.w, imageBuffers.h,
        1,
        imageBuffers.pitch);

    TessCancelData tessCancelData;
    tessCancelData.textDesc.cancel = tessCancelFunc;
    tessCancelData.textDesc.cancel_this = &tessCancelData;
    tessCancelData.progressTracker = &progressTracker;

    tess->Recognize(&tessCancelData.textDesc);

    progressTracker.finish();

    std::lock_guard<std::mutex> guard(link.lock);
    if (link.terminateJobs)
        return;

    std::unique_ptr<char[]> text(tess->GetUTF8Text());
    if (!text)
        return;

    // Initialize textLen with 0 rather than {} to avoid an error
    // in old GCC versions (< 5.2):
    //    error: braces around scalar initializer for type
    JobResult jobResult {std::move(text), 0, {}};
    dpso::str::prettifyOcrText(
        jobResult.text.get(), &jobResult.textLen);
    std::strcpy(jobResult.timestamp, job.timestamp);

    link.results.push_back(std::move(jobResult));
}


static void threadLoop()
{
    while (true) {
        Job job;

        {
            std::lock_guard<std::mutex> guard(link.lock);

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


int dpsoQueueJob(
    int x, int y, int w, int h, int jobFlags)
{
    updateTessLangsStr();
    if (tessLangsStr.empty())
        return false;

    const dpso::Rect screenshotRect {x, y, w, h};
    START_TIMING(takeScreenshot);
    std::unique_ptr<dpso::backend::Screenshot> screenshot(
        dpso::backend::getBackend().takeScreenshot(screenshotRect));
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
        return true;

    Job job {std::move(screenshot), tessLangsStr, {}, jobFlags};

    const auto time = std::time(nullptr);
    if (const auto* tm = std::localtime(&time))
        if (std::strftime(
                job.timestamp, timestampBufSize,
                "%Y-%m-%d %H:%M:%S", tm) == 0)
            job.timestamp[0] = 0;

    setCLocale();

    {
        std::lock_guard<std::mutex> guard(link.lock);

        link.jobQueue.push(std::move(job));

        ++link.progress.totalJobs;
        link.progressIsNew = true;
    }

    return true;
}


void dpsoGetProgress(struct DpsoProgress* progress, int* isNew)
{
    std::lock_guard<std::mutex> guard(link.lock);

    if (progress)
        *progress = link.progress;

    if (isNew)
        *isNew = link.progressIsNew;

    link.progressIsNew = false;
}


int dpsoGetJobsPending(void)
{
    std::lock_guard<std::mutex> guard(link.lock);
    return link.jobsPending();
}


static std::vector<JobResult> fetchedResults;
static std::vector<DpsoJobResult> returnResults;


int dpsoFetchResults(int fetchChain)
{
    std::lock_guard<std::mutex> guard(link.lock);

    if (link.results.empty()
            || (fetchChain && link.jobsPending()))
        return false;

    fetchedResults.clear();
    fetchedResults.swap(link.results);

    returnResults.clear();
    returnResults.reserve(fetchedResults.size());
    for (const auto& result : fetchedResults)
        returnResults.push_back(
            {result.text.get(), result.textLen, result.timestamp});

    return true;
}


void dpsoGetFetchedResults(
    const struct DpsoJobResult** results, int* numResults)
{
    if (results) {
        // vector::data() is not guaranteed to return null for an
        // empty vector
        if (returnResults.empty())
            *results = nullptr;
        else
            *results = &returnResults[0];
    }

    if (numResults)
        *numResults = returnResults.size();
}


static void waitJobsToFinish()
{
    while (true) {
        {
            std::lock_guard<std::mutex> guard(link.lock);
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
        std::lock_guard<std::mutex> guard(link.lock);
        link.waitingForResults = true;
        link.waitingProgressCallback = progressCallback;
        link.waitingUserData = userData;
    }

    waitJobsToFinish();

    std::lock_guard<std::mutex> guard(link.lock);
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
        std::lock_guard<std::mutex> guard(link.lock);

        link.clearJobQueue();
        link.terminateJobs = true;

        if (link.waitingForResults)
            // dpsoWaitForResults() will set terminateJobs to false.
            return;
    }

    waitJobsToFinish();

    // Locking here is not actually necessary since the idling
    // background thread don't access variables we use below.
    std::lock_guard<std::mutex> guard(link.lock);

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
    tess = new tesseract::TessBaseAPI();
    restoreLocale();

    assert(langs.empty());
    assert(numActiveLangs == 0);
    cacheLangs();
    assert(!tessLangsStrValid);

    assert(!link.jobsPending());
    link.reset();

    bgThread = std::thread(threadLoop);

    fetchedResults.clear();
    returnResults.clear();
}


void shutdown()
{
    langs.clear();
    numActiveLangs = 0;
    tessLangsStrValid = false;

    dpsoTerminateJobs();

    {
        std::lock_guard<std::mutex> guard(link.lock);
        link.terminateThread = true;
    }
    bgThread.join();
    link.terminateThread = false;

    setCLocale();
    delete tess;
    restoreLocale();
}


}
}
