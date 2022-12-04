
#include "ocr_engine/tesseract/tesseract_ocr_engine_creator.h"

#include <cstring>

#include "tesseract/baseapi.h"

#include "ocr_engine/ocr_engine_creator.h"
#include "ocr_engine/tesseract/tesseract_ocr_engine.h"


namespace dpso {
namespace ocr {


// Note that we use TessBaseAPI::Version() instead of macros
// (TESSERACT_VERSION_STR, TESSERACT_MAJOR_VERSION, etc.) since we
// need the runtime version.


static std::string getMajorVersionString()
{
    const auto* v = tesseract::TessBaseAPI::Version();
    const auto* dotPos = std::strchr(v, '.');
    return dotPos ? std::string{v, dotPos} : v;
}


class TesseractOcrEngineCreator : public OcrEngineCreator {
public:
    TesseractOcrEngineCreator()
        : info{
            "tesseract_" + getMajorVersionString(),
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
}
