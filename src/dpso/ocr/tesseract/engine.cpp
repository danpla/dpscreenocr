
#include "ocr/tesseract/engine.h"

#include <cstring>

#include <tesseract/baseapi.h>
// We no longer support Tesseract versions prior to 4.1.0 since they
// depend on the "C" locale.
#if !defined(TESSERACT_MAJOR_VERSION) \
        || TESSERACT_MAJOR_VERSION < 4 \
        || (TESSERACT_MAJOR_VERSION == 4 \
            && TESSERACT_MINOR_VERSION < 1)
    #error "Tesseract >= 4.1.0 is required"
#endif

#include "ocr/engine.h"
#include "ocr/tesseract/lang_manager.h"
#include "ocr/tesseract/recognizer.h"


namespace dpso::ocr::tesseract {
namespace {


// Note that we use TessBaseAPI::Version() instead of macros
// (TESSERACT_VERSION_STR, TESSERACT_MAJOR_VERSION, etc.) since we
// need the runtime version.


std::string getMajorVersionString()
{
    const auto* v = ::tesseract::TessBaseAPI::Version();
    const auto* dotPos = std::strchr(v, '.');
    return dotPos ? std::string{v, dotPos} : v;
}


EngineInfo::DataDirPreference getDataDirPreference()
{
    #if DPSO_USE_DEFAULT_TESSERACT_DATA_PATH
    return EngineInfo::DataDirPreference::preferDefault;
    #else
    return EngineInfo::DataDirPreference::preferExplicit;
    #endif
}


class TesseractEngine : public Engine {
public:
    TesseractEngine()
        : info{
            "tesseract_" + getMajorVersionString(),
            "Tesseract",
            ::tesseract::TessBaseAPI::Version(),
            getDataDirPreference(),
            getDataDirPreference()
                    == EngineInfo::DataDirPreference::preferExplicit
                && hasLangManager()}
    {
    }

    const EngineInfo& getInfo() const override
    {
        return info;
    }

    std::unique_ptr<Recognizer> createRecognizer(
        const char* dataDir) const override
    {
        return tesseract::createRecognizer(dataDir);
    }

    std::unique_ptr<LangManager> createLangManager(
        const char* dataDir,
        const char* userAgent,
        const char* infoFileUrl) const override
    {
        return tesseract::createLangManager(
            dataDir, userAgent, infoFileUrl);
    }
private:
    EngineInfo info;
};


}


const Engine& getEngine()
{
    static TesseractEngine engine;
    return engine;
}


}
