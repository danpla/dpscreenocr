
#include "ocr_engine/tesseract/tesseract_ocr_engine_creator.h"

#include "ocr_engine/ocr_engine_creator.h"
#include "ocr_engine/tesseract/tesseract_ocr_engine.h"


namespace dpso {


class TesseractOcrEngineCreator : public OcrEngineCreator {
public:
    TesseractOcrEngineCreator()
        : info{
            "tesseract",
            "Tesseract",
            #if defined(__unix__) && !defined(__APPLE__)
            OcrEngineInfo::DataDirPreference::preferDefault
            #else
            OcrEngineInfo::DataDirPreference::preferExplicit
            #endif
        }
    {
    }

    const OcrEngineInfo& getInfo() const override
    {
        return info;
    }

    std::unique_ptr<OcrEngine> create(
        const OcrEngineArgs& args) const override
    {
        return createTesseractOcrEngine(args);
    }
private:
    OcrEngineInfo info;
};


const OcrEngineCreator& getTesseractOcrEngineCreator()
{
    static TesseractOcrEngineCreator creator;
    return creator;
}


}
