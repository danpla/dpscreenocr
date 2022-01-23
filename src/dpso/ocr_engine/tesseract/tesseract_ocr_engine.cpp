
#include "ocr_engine/tesseract/tesseract_ocr_engine.h"

#include <cassert>
#include <string>
#include <utility>
#include <vector>

#include "tesseract/baseapi.h"
#include "tesseract/ocrclass.h"
#if !defined(TESSERACT_MAJOR_VERSION) || TESSERACT_MAJOR_VERSION < 5
#include "tesseract/genericvector.h"
#include "tesseract/strngs.h"
#endif

#include "ocr_engine/tesseract/lang_names.h"
#include "ocr_engine/tesseract/tesseract_result_text.h"
#include "ocr_engine/tesseract/utils.h"


namespace dpso {
namespace {


class TesseractOcr : public OcrEngine {
public:
    TesseractOcr();

    OcrFeatures getFeatures() const override;

    int getNumLangs() const override;
    const char* getLangCode(int langIdx) const override;
    const char* getDefaultLangCode() const override;
    const char* getLangName(const char* langCode) const override;

    OcrResult recognize(
        const OcrImage& image,
        const std::vector<int>& langIndices,
        OcrFeatures ocrFeatures,
        OcrProgressCallback progressCallback,
        void* progressCallbackUserData) override;
private:
    tesseract::TessBaseAPI tess;
    std::vector<std::string> langCodes;
    std::string tessLangsStr;

    void cacheLangs();
    void fillTessLangsStr(const std::vector<int>& langIndices);
};


}


TesseractOcr::TesseractOcr()
    : tess{}
    , langCodes{}
    , tessLangsStr{}
{
    cacheLangs();
}


OcrFeatures TesseractOcr::getFeatures() const
{
    return ocrFeatureTextSegmentation;
}


int TesseractOcr::getNumLangs() const
{
    return langCodes.size();
}


const char* TesseractOcr::getLangCode(int langIdx) const
{
    return langCodes[langIdx].c_str();
}


const char* TesseractOcr::getDefaultLangCode() const
{
    return "eng";
}


const char* TesseractOcr::getLangName(const char* langCode) const
{
    return getTesseractLangName(langCode);
}


namespace {


struct CancelData {
    #if defined(TESSERACT_MAJOR_VERSION) \
        && TESSERACT_MAJOR_VERSION >= 5
    tesseract::ETEXT_DESC
    #else
    ETEXT_DESC
    #endif
        textDesc;
    OcrProgressCallback progressCallback;
    void* progressCallbackUserData;
    bool cancelled;

    CancelData(
            OcrProgressCallback progressCallback,
            void* progressCallbackUserData)
        : textDesc{}
        , progressCallback{progressCallback}
        , progressCallbackUserData{progressCallbackUserData}
        , cancelled{}
    {
        textDesc.cancel = cancelFunc;
        textDesc.cancel_this = this;
    }

    static bool cancelFunc(void* data, int words)
    {
        (void)words;

        auto* cancelData = static_cast<CancelData*>(data);
        assert(cancelData);

        if (!cancelData->progressCallback)
            return false;

        cancelData->cancelled = !cancelData->progressCallback(
            cancelData->textDesc.progress,
            cancelData->progressCallbackUserData);

        return cancelData->cancelled;
    }
};


}


OcrResult TesseractOcr::recognize(
    const OcrImage& image,
    const std::vector<int>& langIndices,
    OcrFeatures ocrFeatures,
    OcrProgressCallback progressCallback,
    void* progressCallbackUserData)
{
    fillTessLangsStr(langIndices);
    if (tess.Init(nullptr, tessLangsStr.c_str()) != 0)
        return {
            OcrResult::Status::error, "TessBaseAPI::Init() failed"};

    tesseract::PageSegMode pageSegMode;
    if (ocrFeatures & ocrFeatureTextSegmentation)
        pageSegMode = tesseract::PSM_AUTO;
    else
        pageSegMode = tesseract::PSM_SINGLE_BLOCK;
    tess.SetPageSegMode(pageSegMode);

    tess.SetImage(
        image.data, image.width, image.height, 1, image.pitch);

    CancelData cancelData(
        progressCallback, progressCallbackUserData);
    tess.Recognize(&cancelData.textDesc);

    if (cancelData.cancelled)
        return {OcrResult::Status::terminated, nullptr};

    std::unique_ptr<char[]> text{tess.GetUTF8Text()};
    if (!text)
        return {
            OcrResult::Status::error,
            "TessBaseAPI::GetUTF8Text() returned null"};

    const auto textLen = prettifyTesseractText(text.get());

    return {
        OcrResult::Status::success,
        std::unique_ptr<OcrResultText>(
            new TesseractOcrResultText(std::move(text), textLen))};
}


void TesseractOcr::cacheLangs()
{
    // GetAvailableLanguagesAsVector() is broken by design: it uses
    // a data path from the last Init(), while Init() itself requires
    // at least one language (null language is implicit "eng"). As
    // a workaround, we don't check Init() for error: it will fail if
    // "eng" is not available, but the path will remain in TessBaseAPI
    // so GetAvailableLanguagesAsVector() can use it.
    //
    // The only alternative is to make sure that "eng" is available,
    // e.g. make it a dependency of our application package when
    // distributing on Unix-like systems.
    //
    // Writing our own routine to collect the list of "*.tessdata" is
    // not an option, since the data path may be different on various
    // Unix-like systems. It's hardcoded into the Tesseract library,
    // but there's no API to retrieve it; you can only use it
    // implicitly by passing null as the datapath argument to Init().
    tess.Init(nullptr, nullptr);

    #if defined(TESSERACT_MAJOR_VERSION) \
        && TESSERACT_MAJOR_VERSION >= 5

    tess.GetAvailableLanguagesAsVector(&langCodes);

    #else

    GenericVector<STRING> tessLangCodes;
    tess.GetAvailableLanguagesAsVector(&tessLangCodes);

    for (int i = 0; i < langCodes.size(); ++i)
        langCodes.push_back({langCode.c_str(), langCode.size()});

    #endif

    for (std::size_t i = 0; i < langCodes.size();) {
        const auto& langCode = langCodes[i];
        if (langCode == "osd" || langCode == "equ")
            langCodes.erase(langCodes.begin() + i);
        else
            ++i;
    }
}


void TesseractOcr::fillTessLangsStr(
    const std::vector<int>& langIndices)
{
    tessLangsStr.clear();

    for (const auto langIdx : langIndices) {
        if (!tessLangsStr.empty())
            tessLangsStr += '+';

        tessLangsStr += langCodes[langIdx];
    }
}


std::unique_ptr<OcrEngine> createTesseractOcrEngine()
{
    return std::unique_ptr<OcrEngine>(new TesseractOcr{});
}


}
