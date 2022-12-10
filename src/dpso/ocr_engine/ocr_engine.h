
#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>


namespace dpso {
namespace ocr {


class OcrEngineError : public std::runtime_error {
    using runtime_error::runtime_error;
};


struct OcrEngineArgs {
    /**
     * Path to OCR engine data directory.
     *
     * May be empty to use the default path, or if the engine doesn't
     * use external data. See OcrEngineInfo::DataDirPreference for the
     * details.
     */
    const char* dataDir;
};


/**
 * Grayscale image for OCR.
 */
struct OcrImage {
    const std::uint8_t* data;
    int width;
    int height;
    int pitch;
};


enum OcrFeature {
    ocrFeatureTextSegmentation = 1 << 0
};


/**
 * Combination or OcrFeature flags.
 */
using OcrFeatures = unsigned;


/**
 * Progress callback.
 *
 * \param progress OCR progress in percents from 0 to 100.
 * \param progress User data.
 *
 * \return true to continue OCR, false to terminate.
 */
using OcrProgressCallback = bool (*)(int progress, void* userData);


struct OcrResult {
    enum class Status {
        /**
         * OCR was successful.
         *
         * The text field holds the recognized text.
         */
        success,

        /**
         * OCR was terminated from the progress callback.
         *
         * The text is empty.
         */
        terminated,

        /**
         * OCR error.
         *
         * The text holds an error message.
         */
        error
    };

    Status status;
    std::string text;
};


class OcrEngine {
public:
    virtual ~OcrEngine() = default;

    virtual OcrFeatures getFeatures() const = 0;

    virtual int getNumLangs() const = 0;

    /**
     * Get language code.
     *
     * A language code is a string that uniquely identify a language,
     * like an ISO 639 code. It may contain ASCII alphanumeric
     * characters, hyphens, and underscores.
     *
     * The returned pointer should remain valid for the lifetime of
     * the engine.
     *
     * The codes are not required to be sorted.
     */
    virtual const char* getLangCode(int langIdx) const = 0;

    /**
     * Get default language code.
     *
     * The main purpose of default language code is to be used in GUI
     * to select a language when a program starts for the first time.
     *
     * The default language is normally English, but may be different
     * in case the OCR engine is designed for a specific group of
     * languages.
     *
     * The method should not depend on whether the corresponding
     * language pack is currently available. The result should be an
     * empty string if the OCR engine has no meaningful default
     * language.
     */
    virtual const char* getDefaultLangCode() const = 0;

    /**
     * Get language name.
     *
     * Returns the language name for the given code, or null if the
     * code is not known. The method should not depend on whether the
     * corresponding language pack is currently available. If
     * possible, the name should be in English.
     *
     * The returned pointer should remain valid for the lifetime of
     * the engine.
     */
    virtual const char* getLangName(const char* langCode) const = 0;

    virtual OcrResult recognize(
        const OcrImage& image,
        const std::vector<int>& langIndices,
        OcrFeatures ocrFeatures,
        OcrProgressCallback progressCallback,
        void* progressCallbackUserData) = 0;
};


}
}
