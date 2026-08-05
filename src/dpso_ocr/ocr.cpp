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

#include "dpso_img/ops.h"
#include "dpso_img/pnm.h"

#include "dpso_utils/error_set.h"
#include "dpso_utils/geometry.h"
#include "dpso_utils/str.h"
#include "dpso_utils/strftime.h"
#include "dpso_utils/synchronized.h"
#include "dpso_utils/timing.h"

#include "data_lock.h"
#include "engine/engine.h"
#include "engine/recognizer.h"
#include "engine/recognizer_error.h"


using namespace dpso;


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
    img::ImgUPtr image;
    std::vector<int> langIndices;
    ocr::OcrFeatures ocrFeatures;
    std::string timestamp;
};


struct JobResult {
    ocr::Recognizer::Result ocrResult;
    std::string timestamp;
};


// Link between main and background threads.
struct Link {
    std::condition_variable threadActionCondVar;
    std::condition_variable jobsDoneCondVar;

    std::queue<Job> jobQueue;
    bool jobActive;

    DpsoOcrProgress progress;

    std::queue<JobResult> results;

    bool terminateJobs;
    bool terminateThread;

    bool jobsPending() const
    {
        return !jobQueue.empty() || jobActive;
    }
};


}


struct DpsoOcr {
    ocr::DataLockObserver dataLockObserver;

    std::unique_ptr<ocr::Recognizer> recognizer;

    std::string defaultLangCode;
    std::vector<Lang> langs;
    int numActiveLangs;

    Synchronized<Link> link;
    std::thread thread;

    std::vector<std::uint8_t> imgBuffers[2];
    img::Upscale upscale;
    img::UnsharpMask unsharpMask;
    bool dumpDebugImages;

    std::size_t numPendingResults;
    std::queue<JobResult> results;
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

    for (int i{}; i < ocr.recognizer->getNumLangs(); ++i)
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
                >= ocr::Engine::getCount()) {
        setError("engineIdx is out of bounds");
        return nullptr;
    }

    const auto& ocrEngine = ocr::Engine::get(engineIdx);

    // We don't use OcrUPtr here because dpsoOcrDelete() expects
    // a joinable thread.
    auto ocr = std::make_unique<DpsoOcr>();

    ocr->dataLockObserver = ocr::DataLockObserver{
        ocrEngine.getInfo().id,
        dataDir,
        [&ocr = *ocr]
        {
            waitJobsToFinish(ocr);
        },
        [&ocr = *ocr]
        {
            try {
                ocr.recognizer->reloadLangs();
            } catch (ocr::RecognizerError&) {
                return;
            }

            reloadLangs(ocr);
        }
    };

    if (ocr->dataLockObserver.getIsDataLocked()) {
        setError("OCR data is locked");
        return nullptr;
    }

    try {
        ocr->recognizer = ocrEngine.createRecognizer(dataDir);
    } catch (ocr::RecognizerError& e) {
        setError("Can't create recognizer: {}", e.what());
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
        const auto link = ocr->link.getLock();
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


static ocr::Recognizer::Image prepareImage(
    DpsoOcr& ocr, const DpsoImg* image)
{
    assert(image);

    const auto imageW = dpsoImgGetWidth(image);
    const auto imageH = dpsoImgGetHeight(image);

    const auto scale = 4;
    const auto bufferW = imageW * scale;
    const auto bufferH = imageH * scale;
    const auto bufferPitch = bufferW;

    for (auto& buffer : ocr.imgBuffers)
        buffer.resize(bufferH * bufferPitch);

    const std::uint8_t* graySrc;
    int graySrcPitch;
    if (const auto pxFormat = dpsoImgGetPxFormat(image);
            pxFormat == DpsoPxFormatGrayscale) {
        graySrc = dpsoImgGetConstData(image);
        graySrcPitch = dpsoImgGetPitch(image);
    } else {
        DPSO_START_TIMING(toGray);
        img::toGray(
            dpsoImgGetConstData(image),
            dpsoImgGetPitch(image),
            pxFormat,
            ocr.imgBuffers[0].data(),
            bufferPitch,
            imageW,
            imageH);
        DPSO_END_TIMING(
            toGray,
            "{} to grayscale ({}x{} px)",
            dpsoPxFormatToStr(pxFormat), imageW, imageH);

        graySrc = ocr.imgBuffers[0].data();
        graySrcPitch = bufferPitch;
    }

    if (ocr.dumpDebugImages) {
        const auto pxFormat = dpsoImgGetPxFormat(image);
        img::savePnm(
            str::format(
                "dpso_debug_1_original_{}{}",
                dpsoPxFormatToStr(pxFormat),
                img::getPnmExt(pxFormat)),
            pxFormat,
            dpsoImgGetConstData(image),
            imageW, imageH, dpsoImgGetPitch(image));

        img::savePnm(
            "dpso_debug_2_grayscale.pgm",
            DpsoPxFormatGrayscale,
            graySrc, imageW, imageH, graySrcPitch);
    }

    DPSO_START_TIMING(imageResizing);
    ocr.upscale(
        graySrc, imageW, imageH, graySrcPitch,
        ocr.imgBuffers[1].data(), bufferW, bufferH, bufferPitch);
    DPSO_END_TIMING(
        imageResizing,
        "Image resizing ({}x{} px -> {}x{} px, x{})",
        imageW, imageH, bufferW, bufferH, scale);

    if (ocr.dumpDebugImages)
        img::savePnm(
            "dpso_debug_3_resize.pgm",
            DpsoPxFormatGrayscale,
            ocr.imgBuffers[1].data(), bufferW, bufferH, bufferPitch);

    const auto unsharpMaskRadius = 10;

    DPSO_START_TIMING(unsharpMasking);
    ocr.unsharpMask(
        ocr.imgBuffers[1].data(), bufferPitch,
        ocr.imgBuffers[0].data(), bufferPitch,
        bufferW, bufferH,
        unsharpMaskRadius);
    DPSO_END_TIMING(
        unsharpMasking,
        "Unsharp masking (radius={}, {}x{} px)",
        unsharpMaskRadius, bufferW, bufferH);

    if (ocr.dumpDebugImages)
        img::savePnm(
            "dpso_debug_4_unsharp_mask.pgm",
            DpsoPxFormatGrayscale,
            ocr.imgBuffers[0].data(), bufferW, bufferH, bufferPitch);

    return {ocr.imgBuffers[0].data(), bufferW, bufferH, bufferPitch};
}


static JobResult processJob(DpsoOcr& ocr, const Job& job)
{
    auto ocrResult = ocr.recognizer->recognize(
        prepareImage(ocr, job.image.get()),
        job.langIndices,
        job.ocrFeatures,
        [&]
        {
            return !ocr.link.getLock()->terminateJobs;
        });

    return {std::move(ocrResult), job.timestamp};
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
            ++link->progress.curJob;
        }

        auto jobResult = processJob(ocr, job);

        const auto link = ocr.link.getLock();

        link->results.push(std::move(jobResult));

        link->jobActive = false;
        if (link->jobQueue.empty()) {
            link->progress = {};
            link->jobsDoneCondVar.notify_one();
        }
    }
}


bool dpsoOcrQueueJob(
    DpsoOcr* ocr, DpsoImg** img, DpsoOcrJobFlags flags)
{
    if (!img) {
        setError("img is null");
        return false;
    }

    if (!*img) {
        setError("*img is null");
        return false;
    }

    img::ImgUPtr image{std::exchange(*img, {})};

    if (!ocr) {
        setError("ocr is null");
        return false;
    }

    if (ocr->numActiveLangs == 0) {
        setError("No active languages");
        return false;
    }

    if (ocr->dataLockObserver.getIsDataLocked()) {
        setError("OCR data is locked");
        return false;
    }

    ocr::OcrFeatures ocrFeatures{};
    if (flags & dpsoOcrJobTextSegmentation)
        ocrFeatures |= ocr::ocrFeatureTextSegmentation;

    Job job{
        std::move(image),
        getActiveLangIndices(*ocr),
        ocrFeatures,
        createTimestamp()};

    ++ocr->numPendingResults;

    const auto link = ocr->link.getLock();

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
        && a->curJob == b->curJob
        && a->totalJobs == b->totalJobs;
}


void dpsoOcrGetProgress(const DpsoOcr* ocr, DpsoOcrProgress* progress)
{
    if (!ocr || !progress)
        return;

    // Check numPendingResults as a small optimization to avoid
    // link.getLock().
    *progress = ocr->numPendingResults == 0
        ? DpsoOcrProgress{} : ocr->link.getLock()->progress;
}


bool dpsoOcrHasPendingResults(const DpsoOcr* ocr)
{
    return ocr && ocr->numPendingResults > 0;
}


bool dpsoOcrGetResult(DpsoOcr* ocr, DpsoOcrJobResult* result)
{
    if (!ocr || ocr->numPendingResults == 0 || !result)
        return false;

    if (!ocr->results.empty())
        ocr->results.pop();

    if (ocr->results.empty()) {
        ocr->results.swap(ocr->link.getLock()->results);
        if (ocr->results.empty())
            return false;
    }

    const auto& r = ocr->results.front();
    *result = {
        r.ocrResult.text.c_str(),
        r.ocrResult.text.size(),
        r.timestamp.c_str()};

    --ocr->numPendingResults;

    return true;
}


void dpsoOcrTerminateJobs(DpsoOcr* ocr)
{
    if (!ocr || ocr->numPendingResults == 0)
        return;

    ocr->numPendingResults = 0;
    ocr->results = {};

    {
        const auto link = ocr->link.getLock();
        link->jobQueue = {};
        link->terminateJobs = true;
    }

    waitJobsToFinish(*ocr);

    const auto link = ocr->link.getLock();
    link->results = {};
    link->terminateJobs = false;
}
