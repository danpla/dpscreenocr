#include "engine/tesseract/lang_utils.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <initializer_list>

#include <tesseract/baseapi.h>
#if TESSERACT_MAJOR_VERSION < 5
#include <tesseract/genericvector.h>
#include <tesseract/strngs.h>
#endif

#include "dpso_utils/os.h"

#include "engine/error.h"


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
    } catch (os::Error& e) {
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
    // See https://github.com/tesseract-engine/tesseract/issues/1073
    //
    // We can't get around the problem by writing our own routine that
    // will collect the list of "*.traineddata" files. The data path
    // may be different on various Unix-like systems (it's hardcoded
    // at compilation time; our dataDir is empty in this case), and
    // you can only get it via GetDatapath(), which also requires the
    // same dummy Init() call.
    tess.Init(sysDataDir.c_str(), nullptr);

    std::vector<std::string> result;

    #if TESSERACT_MAJOR_VERSION >= 5

    // In Tesseract 5.5.0, GetAvailableLanguagesAsVector() was
    // rewritten to use std::filesystem, but due to the lack of any
    // exception handling, the method leaks filesystem_error (e.g. if
    // the directory does not exist).
    //
    // See https://github.com/tesseract-ocr/tesseract/issues/4364
    try {
        tess.GetAvailableLanguagesAsVector(&result);
    } catch (std::filesystem::filesystem_error&) {
        return result;
    }

    result.erase(
        std::remove_if(
            result.begin(), result.end(),
            [](const std::string& lang)
            {
                return isIgnoredLang(lang.c_str());
            }),
        result.end());

    // In Tesseract 5.5.0, GetAvailableLanguagesAsVector() uses
    // incorrect file extension handling, only checking if a filename
    // has a ".traineddata" substring instead of strictly ending with
    // it. As a result, the list can contain duplicates and entries
    // that refer to ".traineddata***" rather than ".traineddata"
    // files.
    //
    // See https://github.com/tesseract-ocr/tesseract/issues/4416
    if (*dataDir && std::strcmp(tess.Version(), "5.5.0") == 0) {
        // Drop duplicates that can occur when several files with the
        // same name have ".traineddata" in their extension, e.g.,
        // "eng.traineddata" and "eng.traineddata.sha256". Note that
        // the result of GetAvailableLanguagesAsVector() is sorted.
        result.erase(
            std::unique(result.begin(), result.end()), result.end());

        // Drop entries that refer to ".traineddata***" rather than
        // real ".traineddata" files.
        result.erase(
            std::remove_if(
                result.begin(), result.end(),
                [&](const std::string& lang)
                {
                    namespace fs = std::filesystem;

                    return !fs::exists(
                        fs::u8path(dataDir)
                        / fs::u8path(lang + traineddataExt));
                }),
            result.end());
    }

    #else

    GenericVector<STRING> tessLangCodes;
    tess.GetAvailableLanguagesAsVector(&tessLangCodes);

    for (int i = 0; i < tessLangCodes.size(); ++i) {
        const auto& langCode = tessLangCodes[i];
        if (!isIgnoredLang(langCode.c_str()))
            result.push_back(
                {
                    langCode.c_str(),
                    static_cast<std::size_t>(langCode.size())});
    }

    #endif

    return result;
}


}
