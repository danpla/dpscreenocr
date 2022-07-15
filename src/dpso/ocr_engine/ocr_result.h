
#pragma once

#include <cstddef>
#include <memory>


namespace dpso {


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

    OcrResult(const OcrResult& other) = delete;
    OcrResult& operator=(const OcrResult& other) = delete;

    OcrResult(OcrResult&& other) noexcept;
    OcrResult& operator=(OcrResult&& other) noexcept;

    Status getStatus() const;

    /**
     * Get null-terminated text.
     */
    const char* getText() const;

    /**
     * Get length of the text excluding the null terminator.
     */
    std::size_t getTextLen() const;
private:
    Status status;
    std::unique_ptr<OcrResultText> text;
};


}
