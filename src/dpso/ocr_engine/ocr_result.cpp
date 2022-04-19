
#include "ocr_engine/ocr_result.h"

#include <utility>

#include "ocr_engine/ocr_result_text.h"


namespace dpso {


OcrResult::OcrResult(Status status, const char* text)
    : status{status}
    , text{text ? OcrResultText::create(text) : nullptr}
{
}


OcrResult::OcrResult(
        Status status, std::unique_ptr<OcrResultText> text)
    : status{status}
    , text{std::move(text)}
{
}


OcrResult::~OcrResult() = default;
OcrResult::OcrResult(OcrResult&&) noexcept = default;
OcrResult& OcrResult::operator=(OcrResult&&) noexcept = default;


OcrResult::Status OcrResult::getStatus() const
{
    return status;
}


const char* OcrResult::getText() const
{
    return text ? text->getData() : "";
}


std::size_t OcrResult::getTextLen() const
{
    return text ? text->getLen() : 0;
}


}
