
#include "ocr/tesseract/recognizer.h"

#include <cassert>
#include <cstring>
#include <string>
#include <vector>

#include <tesseract/baseapi.h>
#include <tesseract/ocrclass.h>

#include "dpso_utils/os.h"

#include "ocr/recognizer_error.h"
#include "ocr/tesseract/lang_names.h"
#include "ocr/tesseract/lang_utils.h"
#include "ocr/tesseract/utils.h"


namespace dpso::ocr::tesseract {
namespace {


class TesseractRecognizer : public Recognizer {
public:
    explicit TesseractRecognizer(const char* dataDir)
        : dataDir{dataDir}
    {
        reloadLangCodes();
    }

    OcrFeatures getFeatures() const override
    {
        return ocrFeatureTextSegmentation;
    }

    int getNumLangs() const override
    {
        return langCodes.size();
    }

    std::string getLangCode(int langIdx) const override
    {
        return langCodes[langIdx];
    }

    std::string getDefaultLangCode() const override
    {
        return "eng";
    }

    std::string getLangName(int langIdx) const override
    {
        const auto* name = tesseract::getLangName(
            langCodes[langIdx].c_str());
        return name ? name : "";
    }

    void reloadLangs() override
    {
        reloadLangCodes();
    }

    OcrResult recognize(
        const OcrImage& image,
        const std::vector<int>& langIndices,
        OcrFeatures ocrFeatures,
        const ProgressHandler& progressHandler) override;
private:
    std::string dataDir;
    ::tesseract::TessBaseAPI tess;
    std::vector<std::string> langCodes;

    void reloadLangCodes()
    {
        try {
            langCodes = getAvailableLangs(dataDir.c_str());
        } catch (Error& e) {
            throw RecognizerError{
                std::string{"Can't get available languages: "}
                + e.what()};
        }
    }
};


struct CancelData {
    #if TESSERACT_MAJOR_VERSION >= 5
    ::tesseract::ETEXT_DESC
    #else
    ETEXT_DESC
    #endif
        textDesc;
    const Recognizer::ProgressHandler& progressHandler;
    bool canceled;

    explicit CancelData(
            const Recognizer::ProgressHandler& progressHandler)
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

        cancelData->canceled = !cancelData->progressHandler(
            cancelData->textDesc.progress);

        return cancelData->canceled;
    }
};


bool isVertical(const char* langCode)
{
    const auto* s = std::strrchr(langCode, '_');
    return s && std::strcmp(s + 1, "vert") == 0;
}


OcrResult TesseractRecognizer::recognize(
    const OcrImage& image,
    const std::vector<int>& langIndices,
    OcrFeatures ocrFeatures,
    const ProgressHandler& progressHandler)
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

    std::string sysDataDir;
    try {
        sysDataDir = os::convertUtf8PathToSys(dataDir.c_str());
    } catch (std::runtime_error& e) {
        return {
            OcrResult::Status::error,
            std::string{"Can't convert dataDir to system encoding: "}
                + e.what()};
    }

    if (tess.Init(sysDataDir.c_str(), tessLangsStr.c_str()) != 0)
        return {
            OcrResult::Status::error, "TessBaseAPI::Init() failed"};

    ::tesseract::PageSegMode pageSegMode;

    if (ocrFeatures & ocrFeatureTextSegmentation)
        pageSegMode = ::tesseract::PSM_AUTO;
    else if (numVerticalLangs == 0)
        // PSM_SINGLE_BLOCK implies horizontal text.
        pageSegMode = ::tesseract::PSM_SINGLE_BLOCK;
    else if (numVerticalLangs == langIndices.size())
        pageSegMode = ::tesseract::PSM_SINGLE_BLOCK_VERT_TEXT;
    else
        // When we have both vertical and horizontal languages,
        // forcing segmentation is the best we can do: at least
        // Tesseract will be able to pick the right language based
        // on the actual orientation of the text in each block.
        // Using a specific PSM_SINGLE_BLOCK_* instead will 100%
        // break OCR of text with the opposite orientation.
        pageSegMode = ::tesseract::PSM_AUTO;

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

    const auto textLen = prettifyText(text.get());

    return {OcrResult::Status::success, {text.get(), textLen}};
}


}


std::unique_ptr<Recognizer> createRecognizer(const char* dataDir)
{
    return std::make_unique<TesseractRecognizer>(dataDir);
}


}
