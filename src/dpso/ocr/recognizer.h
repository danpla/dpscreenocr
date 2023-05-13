
#pragma once

#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>


namespace dpso::ocr {


// Grayscale image for OCR.
struct OcrImage {
    const std::uint8_t* data;
    int width;
    int height;
    int pitch;
};


enum OcrFeature {
    ocrFeatureTextSegmentation = 1 << 0
};


using OcrFeatures = unsigned;


struct OcrResult {
    enum class Status {
        // OCR was successful. The text field holds the recognized
        // text.
        success,

        // OCR was terminated from the progress callback. The text is
        // empty.
        terminated,

        // OCR error. The text holds an error message.
        error
    };

    Status status;
    std::string text;
};


class Recognizer {
public:
    // The progress is in percents from 0 to 100. Returns false to
    // terminate OCR.
    using ProgressHandler = std::function<bool(int progress)>;

    virtual ~Recognizer() = default;

    virtual OcrFeatures getFeatures() const = 0;

    virtual int getNumLangs() const = 0;

    // See dpsoOcrGetLangCode()
    virtual std::string getLangCode(int langIdx) const = 0;

    // See dpsoOcrGetDefaultLangCode()
    virtual std::string getDefaultLangCode() const = 0;

    // See dpsoOcrGetLangName()
    virtual std::string getLangName(int langIdx) const = 0;

    virtual void reloadLangs() = 0;

    virtual OcrResult recognize(
        const OcrImage& image,
        const std::vector<int>& langIndices,
        OcrFeatures ocrFeatures,
        const ProgressHandler& progressHandler) = 0;
};


}
