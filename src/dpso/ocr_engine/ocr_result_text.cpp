
#include "ocr_engine/ocr_result_text.h"

#include <string>


namespace dpso {
namespace {


class SimpleOcrResultText : public OcrResultText {
public:
    explicit SimpleOcrResultText(const char* text)
        : text{text}
    {
    }

    const char* getData() const override
    {
        return text.data();
    }

    virtual std::size_t getLen() const override
    {
        return text.size();
    }
private:
    std::string text;
};


}


std::unique_ptr<OcrResultText> OcrResultText::create(const char* text)
{
    return std::unique_ptr<OcrResultText>(
        new SimpleOcrResultText(text));
}


}
