
#include "ocr/tesseract/lang_utils.h"

#include <algorithm>
#include <cstring>
#include <initializer_list>

#include <tesseract/baseapi.h>
#if TESSERACT_MAJOR_VERSION < 5
#include <tesseract/genericvector.h>
#include <tesseract/strngs.h>
#endif

#include "dpso_utils/os.h"

#include "ocr/error.h"


namespace dpso::ocr::tesseract {


const char* const traineddataExt = ".traineddata";


bool isIgnoredLang(const char* lang)
{
    for (const auto* ignoredLang : {"equ", "osd"})
        if (std::strcmp(lang, ignoredLang) == 0)
            return true;

    return false;
}


std::vector<std::string> getAvailableLangs(const char* dataDir)
{
    std::string sysDataDir;
    try {
        sysDataDir = os::convertUtf8PathToSys(dataDir);
    } catch (std::runtime_error& e) {
        throw Error{
            std::string{"Can't convert dataDir to system encoding: "}
            + e.what()};
    }

    ::tesseract::TessBaseAPI tess;

    // GetAvailableLanguagesAsVector() is broken by design: it uses
    // a data path from the last Init(), while Init() itself requires
    // at least one language (null language is implicit "eng" in
    // versions before 5). As a workaround, we don't check Init() for
    // error: it will fail if "eng" is not available, but the path
    // will remain in TessBaseAPI so GetAvailableLanguagesAsVector()
    // can use it.
    //
    // The only alternative is to make sure that "eng" is available,
    // e.g. make it a dependency of our application package when
    // distributing on Unix-like systems.
    //
    // See https://github.com/tesseract-ocr/tesseract/issues/1073
    //
    // We can't get around the problem by writing our own routine that
    // will collect the list of "*.traineddata" files. The data path
    // may be different on various Unix-like systems (it's hardcoded
    // at compilation time), and you can only get it via
    // GetDatapath(), which also requires the same dummy Init() call.
    tess.Init(sysDataDir.c_str(), nullptr);

    std::vector<std::string> result;

    #if TESSERACT_MAJOR_VERSION >= 5

    tess.GetAvailableLanguagesAsVector(&result);

    result.erase(
        std::remove_if(
            result.begin(), result.end(),
            [](const std::string& lang)
            {
                return isIgnoredLang(lang.c_str());
            }),
        result.end());

    #else

    GenericVector<STRING> tessLangCodes;
    tess.GetAvailableLanguagesAsVector(&tessLangCodes);

    for (int i = 0; i < tessLangCodes.size(); ++i) {
        const auto& langCode = tessLangCodes[i];
        if (!isIgnoredLang(langCode.c_str()))
            result.push_back(
                {
                    langCode.c_str(),
                    static_cast<std::size_t>(langCode.size())
                });
    }

    #endif

    return result;
}


}
