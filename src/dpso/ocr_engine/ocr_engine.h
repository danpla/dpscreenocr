
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>


namespace dpso {


/**
 * OCR engine error.
 *
 * The exception may be thrown from the OcrEngine constructor.
 */
class OcrEngineError : public std::runtime_error {
    using runtime_error::runtime_error;
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


class OcrResultText;


/**
 * OCR result.
 *
 * In both constructors, the text argument may be null regardless of
 * the status. OcrResult::getText() will return an empty string in
 * this case.
 */
class OcrResult {
public:
    enum class Status {
        /**
         * OCR was successful.
         *
         * getText() returns the recognized text.
         */
        success,

        /**
         * OCR was terminated from progress callback.
         *
         * getText() returns an empty string.
         */
        terminated,

        /**
         * OCR error.
         *
         * getText() returns an error message.
         */
        error
    };

    OcrResult(Status status, const char* text);
    OcrResult(Status status, std::unique_ptr<OcrResultText> text);
    ~OcrResult();

    OcrResult(OcrResult&& other);
    OcrResult& operator=(OcrResult&& other);

    Status getStatus() const;

    const char* getText() const;
    std::size_t getTextLen() const;
private:
    Status status;
    std::unique_ptr<OcrResultText> text;
};


class OcrEngine {
public:
    /**
     * Create OCR engine.
     *
     * \throws OcrEngineError
     */
    static std::unique_ptr<OcrEngine> create();

    OcrEngine() = default;
    virtual ~OcrEngine() = default;

    OcrEngine(const OcrEngine& other) = delete;
    OcrEngine& operator=(const OcrEngine& other) = delete;

    OcrEngine(OcrEngine&& other) = delete;
    OcrEngine& operator=(OcrEngine&& other) = delete;

    /**
     * Get extra OCR features supported by engine.
     *
     * Returns combination of OcrFeature flags.
     */
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
        const std::vector<int> langIndices,
        OcrFeatures ocrFeatures,
        OcrProgressCallback progressCallback,
        void* progressCallbackUserData) = 0;
};


}
