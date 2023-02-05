
#include "ocr_engine/tesseract/tesseract_ocr_engine.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

#include "tesseract/baseapi.h"
// We no longer support Tesseract versions prior to 4.1.0 since they
// depend on the "C" locale.
#if !defined(TESSERACT_MAJOR_VERSION) \
        || TESSERACT_MAJOR_VERSION < 4 \
        || (TESSERACT_MAJOR_VERSION == 4 \
            && TESSERACT_MINOR_VERSION < 1)
    #error "Tesseract >= 4.1.0 is required"
#endif

#include "tesseract/ocrclass.h"
#if TESSERACT_MAJOR_VERSION < 5
#include "tesseract/genericvector.h"
#include "tesseract/strngs.h"
#endif

#include "ocr_engine/tesseract/lang_names.h"
#include "ocr_engine/tesseract/utils.h"


namespace dpso::ocr {
namespace {


class TesseractOcr : public OcrEngine {
public:
    explicit TesseractOcr(const OcrEngineArgs& args);

    OcrFeatures getFeatures() const override;

    int getNumLangs() const override;
    std::string getLangCode(int langIdx) const override;
    std::string getDefaultLangCode() const override;
    std::string getLangName(int langIdx) const override;

    OcrResult recognize(
        const OcrImage& image,
        const std::vector<int>& langIndices,
        OcrFeatures ocrFeatures,
        OcrProgressHandler* progressHandler) override;
private:
    std::string dataDir;
    tesseract::TessBaseAPI tess;
    std::vector<std::string> langCodes;

    bool initTess(const char* langs);
    void cacheLangs();
};


}


TesseractOcr::TesseractOcr(const OcrEngineArgs& args)
    : dataDir{args.dataDir}
    , tess{}
    , langCodes{}
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


std::string TesseractOcr::getLangCode(int langIdx) const
{
    return langCodes[langIdx];
}


std::string TesseractOcr::getDefaultLangCode() const
{
    return "eng";
}


std::string TesseractOcr::getLangName(int langIdx) const
{
    const auto* name = getTesseractLangName(
        langCodes[langIdx].c_str());
    return name ? name : "";
}


namespace {


struct CancelData {
    #if TESSERACT_MAJOR_VERSION >= 5
    tesseract::ETEXT_DESC
    #else
    ETEXT_DESC
    #endif
        textDesc;
    OcrProgressHandler* progressHandler;
    bool canceled;

    explicit CancelData(OcrProgressHandler* progressHandler)
        : textDesc{}
        , progressHandler{progressHandler}
        , canceled{}
    {
        textDesc.cancel = cancelFunc;
        textDesc.cancel_this = this;
    }

    static bool cancelFunc(void* data, int words)
    {
        (void)words;

        auto* cancelData = static_cast<CancelData*>(data);
        assert(cancelData);

        if (!cancelData->progressHandler)
            return false;

        cancelData->canceled = !(*cancelData->progressHandler)(
            cancelData->textDesc.progress);

        return cancelData->canceled;
    }
};


}


static bool isVertical(const char* langCode)
{
    const auto* s = std::strrchr(langCode, '_');
    return s && std::strcmp(s + 1, "vert") == 0;
}


OcrResult TesseractOcr::recognize(
    const OcrImage& image,
    const std::vector<int>& langIndices,
    OcrFeatures ocrFeatures,
    OcrProgressHandler* progressHandler)
{
    std::string tessLangsStr;
    std::size_t numVerticalLangs{};

    for (const auto langIdx : langIndices) {
        const auto& langCode = langCodes[langIdx];

        if (isVertical(langCode.c_str()))
            ++numVerticalLangs;

        if (!tessLangsStr.empty())
            tessLangsStr += '+';

        tessLangsStr += langCode;
    }

    if (!initTess(tessLangsStr.c_str()) != 0)
        return {
            OcrResult::Status::error, "TessBaseAPI::Init() failed"};

    tesseract::PageSegMode pageSegMode{tesseract::PSM_AUTO};

    if (!(ocrFeatures & ocrFeatureTextSegmentation)) {
        if (numVerticalLangs == 0)
            // PSM_SINGLE_BLOCK implies horizontal text.
            pageSegMode = tesseract::PSM_SINGLE_BLOCK;
        else if (numVerticalLangs == langIndices.size())
            pageSegMode = tesseract::PSM_SINGLE_BLOCK_VERT_TEXT;
        else
            // When we have both vertical and horizontal languages,
            // forcing segmentation is the best we can do: at least
            // Tesseract will be able to pick the right language based
            // on the actual orientation of the text in each block.
            // Using a specific PSM_SINGLE_BLOCK_* instead will 100%
            // break OCR of text with the opposite orientation.
            pageSegMode = tesseract::PSM_AUTO;
    }

    tess.SetPageSegMode(pageSegMode);

    tess.SetImage(
        image.data, image.width, image.height, 1, image.pitch);

    CancelData cancelData{progressHandler};
    if (tess.Recognize(&cancelData.textDesc) != 0)
        return {
            OcrResult::Status::error,
            "TessBaseAPI::Recognize() failed"};

    if (cancelData.canceled)
        return {OcrResult::Status::terminated, ""};

    std::unique_ptr<char[]> text{tess.GetUTF8Text()};
    if (!text)
        return {
            OcrResult::Status::error,
            "TessBaseAPI::GetUTF8Text() returned null"};

    const auto textLen = prettifyTesseractText(text.get());

    return {OcrResult::Status::success, {text.get(), textLen}};
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

    #if TESSERACT_MAJOR_VERSION >= 5

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


std::unique_ptr<OcrEngine> createTesseractOcrEngine(
    const OcrEngineArgs& args)
{
    return std::make_unique<TesseractOcr>(args);
}


}
