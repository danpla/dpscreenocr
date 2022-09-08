
#include "ocr_engine/tesseract/tesseract_ocr_engine_creator.h"

#include "tesseract/baseapi.h"

#include "ocr_engine/ocr_engine_creator.h"
#include "ocr_engine/tesseract/tesseract_ocr_engine.h"


namespace dpso {


static std::string getTesseractMajorVersionString()
{
    // We still need to support Tesseract 3.03 (on Ubuntu 14.04),
    // which doesn't provide any version macros. Once we drop it, we
    // can simply use TESSERACT_MAJOR_VERSION.
    std::string v = tesseract::TessBaseAPI::Version();

    const auto dotPos = v.find('.');
    if (dotPos != v.npos)
        v.resize(dotPos);

    return v;
}


class TesseractOcrEngineCreator : public OcrEngineCreator {
public:
    TesseractOcrEngineCreator()
        : info{
            "tesseract_" + getTesseractMajorVersionString(),
            "Tesseract",
            tesseract::TessBaseAPI::Version(),
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
