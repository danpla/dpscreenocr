
#pragma once

#include <memory>


namespace dpso {


/**
 * Text of OcrResult.
 *
 * The purpose of the class is to abstract the way a backed returns
 * text to avoid copying data as std::string in case of C string
 * or a custom string-like class. This optimization is not mandatory,
 * so if there's no sense to bother, simply use the OcrResult
 * constructor taking a C string.
 */
class OcrResultText {
public:
    /**
     * Create an OcrResultText holding a copy of text.
     *
     * Text may be null.
     */
    static std::unique_ptr<OcrResultText> create(const char* text);

    OcrResultText() = default;
    virtual ~OcrResultText() {};

    OcrResultText(const OcrResultText& other) = delete;
    OcrResultText& operator=(const OcrResultText& other) = delete;

    OcrResultText(OcrResultText&& other) = delete;
    OcrResultText& operator=(OcrResultText&& other) = delete;

    /**
     * Get null-terminated text data.
     */
    virtual const char* getData() const = 0;

    /**
     * Get length of the text excluding the null terminator.
     */
    virtual std::size_t getLen() const = 0;
};


}
