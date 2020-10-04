
#pragma once

#include <memory>
#include <utility>

#include "ocr_engine/ocr_result_text.h"


namespace dpso {


class TesseractOcrResultText : public OcrResultText {
public:
    TesseractOcrResultText(
            std::unique_ptr<char[]> text, std::size_t textLen)
        : text{std::move(text)}
        , textLen{textLen}
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


}
