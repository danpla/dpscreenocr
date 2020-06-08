
#include "ocr_engine/tesseract_ocr_engine.h"

#include <cassert>
#include <cctype>
#include <cstring>
#include <string>
#include <utility>

#include "tesseract/baseapi.h"
#include "tesseract/genericvector.h"
#include "tesseract/ocrclass.h"
#include "tesseract/strngs.h"

#include "ocr_engine/ocr_result_text.h"


namespace dpso {
namespace {


class TesseractOcrResultText : public OcrResultText {
public:
    TesseractOcrResultText(
            std::unique_ptr<char[]> text, std::size_t textLen)
        : text {std::move(text)}
        , textLen {textLen}
    {

    }

    const char* getData() const override
    {
        return text.get();
    }

    std::size_t getLen() const override
    {
        return textLen;
    }
private:
    std::unique_ptr<char[]> text;
    std::size_t textLen;
};


class TesseractOcr : public OcrEngine {
public:
    TesseractOcr();

    OcrFeatures getFeatures() const override;

    int getNumLangs() const override;
    const char* getLangCode(int langIdx) const override;

    OcrResult recognize(
        const OcrImage& image,
        const std::vector<int> langIndices,
        OcrFeatures ocrFeatures,
        OcrProgressCallback progressCallback,
        void* progressCallbackUserData) override;
private:
    tesseract::TessBaseAPI tess;
    GenericVector<STRING> langCodes;
    std::string tessLangsStr;

    void cacheLangs();
    void fillTessLangsStr(const std::vector<int> langIndices);
};


}


TesseractOcr::TesseractOcr()
    : tess {}
    , langCodes {}
    , tessLangsStr {}
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
    if (langIdx < 0 || langIdx >= langCodes.size())
        return "";

    return langCodes[langIdx].c_str();
}


namespace {


struct CancelData {
    ETEXT_DESC textDesc;
    OcrProgressCallback progressCallback;
    void* progressCallbackUserData;
    bool cancelled;

    CancelData(
            OcrProgressCallback progressCallback,
            void* progressCallbackUserData)
        : textDesc {}
        , progressCallback {progressCallback}
        , progressCallbackUserData {progressCallbackUserData}
        , cancelled {}
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


// Prettify text returned by Tesseract. The function:
//
//   * Strips leading whitespace.
//   * Splits fi and fl ligatures.
//   * Removes paragraphs consisting of a single space, which are
//       sometimes created when page segmentation is enabled.
//   * Removes the trailing newline (Tesseract adds two).
//
// Returns new length of the text (<= the original).
static std::size_t prettifyText(char* text)
{
    struct Replacement {
        const char* from;
        std::size_t fromLen;
        const char* to;
        std::size_t toLen;

        Replacement(const char* from, const char* to)
            : from {from}
            , fromLen {std::strlen(from)}
            , to {to}
            , toLen {std::strlen(to)}
        {
            assert(fromLen >= toLen);
        }
    };

    // Note that ::from must be >= ::to.
    static const Replacement replacements[] = {
        // Tesseract doesn't seem to generate ligatures other than
        // fi and fl.
        {"\357\254\201", "fi"},
        {"\357\254\202", "fl"},
        // If page segmentation is enabled, Tesseract sometimes
        // generates paragraphs consisting of a single space. If such
        // a paragraph is not the first one, the text may look like
        // "paragraph1\n\n \n\nparagraph3". Removing the "\n \n"
        // sequence will result in a normal paragraph separator
        // (empty line). Leading sequences are removed on trimming.
        {"\n \n", ""},
    };

    const auto* src = text;
    auto* dst = text;

    while (std::isspace(*src))
        ++src;

    while (*src) {
        const auto* oldSrc = src;

        for (const auto& replacement : replacements) {
            if (std::memcmp(
                    src,
                    replacement.from,
                    replacement.fromLen) != 0)
                continue;

            std::memcpy(dst, replacement.to, replacement.toLen);
            src += replacement.fromLen;
            dst += replacement.toLen;

            break;
        }

        if (src == oldSrc)
            *dst++ = *src++;
    }

    // Tesseract ends text with two newlines, while we need only one
    if (dst - text >= 2
            && dst[-1] == '\n'
            && dst[-2] == '\n')
        --dst;

    *dst = 0;

    return dst - text;
}


OcrResult TesseractOcr::recognize(
    const OcrImage& image,
    const std::vector<int> langIndices,
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

    auto* text = tess.GetUTF8Text();
    if (!text)
        return {
            OcrResult::Status::error,
            "TessBaseAPI::GetUTF8Text() returned null"};

    const auto textLen = prettifyText(text);

    return {
        OcrResult::Status::success,
        std::unique_ptr<OcrResultText>(
            new TesseractOcrResultText(
                std::unique_ptr<char[]>(text), textLen))};
}


void TesseractOcr::cacheLangs()
{
    if (tess.Init(nullptr, nullptr) != 0)
        throw OcrEngineError(
            "Can't load languages: TessBaseAPI::Init() failed");

    tess.GetAvailableLanguagesAsVector(&langCodes);

    for (int i = 0; i < langCodes.size();) {
        const auto& langCode = langCodes[i];
        if (langCode == "osd" || langCode == "equ")
            langCodes.remove(i);
        else
            ++i;
    }
}


void TesseractOcr::fillTessLangsStr(
    const std::vector<int> langIndices)
{
    tessLangsStr.clear();

    for (const auto langIdx : langIndices) {
        if (!tessLangsStr.empty())
            tessLangsStr += '+';

        const auto& langCode = langCodes[langIdx];
        tessLangsStr.append(langCode.c_str(), langCode.size());
    }
}


std::unique_ptr<OcrEngine> createTesseractOcrEngine()
{
    return std::unique_ptr<OcrEngine>(new TesseractOcr{});
}


}
