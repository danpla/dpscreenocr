
#include "ocr_engine/tesseract/tesseract_ocr_engine.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <initializer_list>
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
    explicit TesseractOcr(const OcrEngineArgs& args);

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
    std::string dataDir;
    tesseract::TessBaseAPI tess;
    std::vector<std::string> langCodes;
    std::string tessLangsStr;

    bool initTess(const char* langs);
    void cacheLangs();
    void fillTessLangsStr(const std::vector<int>& langIndices);
};


}


TesseractOcr::TesseractOcr(const OcrEngineArgs& args)
    : dataDir{args.dataDir ? args.dataDir : ""}
    , tess{}
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
    if (!initTess(tessLangsStr.c_str()) != 0)
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
        std::make_unique<TesseractOcrResultText>(
            std::move(text), textLen)};
}


bool TesseractOcr::initTess(const char* langs)
{
    return tess.Init(
        dataDir.empty() ? nullptr : dataDir.c_str(), langs) == 0;
}


static bool shouldIgnoreLang(const char* lang)
{
    for (const auto* ignoredLang : {"equ", "osd"})
        if (std::strcmp(lang, ignoredLang) == 0)
            return true;

    return false;
}


void TesseractOcr::cacheLangs()
{
    // GetAvailableLanguagesAsVector() is broken by design: it uses
    // a data path from the last Init(), while Init() itself requires
    // at least one language (null language is implicit "eng" in
    // versions before 5). As a workaround, we don't check Init() for
    // error: it will fail if "eng" is not available, but the path
    // will remain in TessBaseAPI so GetAvailableLanguagesAsVector()
    // can use it.
    //
    // The only alternative is to make sure that "eng" is available,
    // e.g. make it a dependency of our application package when
    // distributing on Unix-like systems.
    //
    // See https://github.com/tesseract-ocr/tesseract/issues/1073
    //
    // We can't get around the problem by writing our own routine that
    // will collect the list of "*.traineddata" files. The data path
    // may be different on various Unix-like systems (it's hardcoded
    // at compilation time), and you can only get it via
    // GetDatapath(), which also requires the same dummy Init() call.
    initTess(nullptr);

    #if defined(TESSERACT_MAJOR_VERSION) \
        && TESSERACT_MAJOR_VERSION >= 5

    tess.GetAvailableLanguagesAsVector(&langCodes);

    langCodes.erase(
        std::remove_if(
            langCodes.begin(), langCodes.end(),
            [](const std::string& lang)
            {
                return shouldIgnoreLang(lang.c_str());
            }),
        langCodes.end());

    #else

    GenericVector<STRING> tessLangCodes;
    tess.GetAvailableLanguagesAsVector(&tessLangCodes);

    for (int i = 0; i < tessLangCodes.size(); ++i) {
        const auto& langCode = tessLangCodes[i];
        if (!shouldIgnoreLang(langCode.c_str()))
            langCodes.push_back(
                {langCode.c_str(),
                    static_cast<std::size_t>(langCode.size())});
    }

    #endif
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


std::unique_ptr<OcrEngine> createTesseractOcrEngine(
    const OcrEngineArgs& args)
{
    return std::make_unique<TesseractOcr>(args);
}


}
