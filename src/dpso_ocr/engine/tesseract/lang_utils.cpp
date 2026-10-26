#include "engine/tesseract/lang_utils.h"

#include <filesystem>
#include <system_error>
#include <utility>

#include <tesseract/baseapi.h>

#include "dpso_utils/str.h"

#include "engine/error.h"


namespace dpso::ocr::tesseract {


const std::string_view traineddataExt{".traineddata"};


bool isIgnoredLang(std::string_view lang)
{
    static const std::string_view ignoredLangs[]{"equ", "osd"};

    for (const auto& ignoredLang : ignoredLangs)
        if (lang == ignoredLang)
            return true;

    return false;
}


const std::string& getDefaultDataDir()
{
    // Both GetDatapath() and GetAvailableLanguagesAsVector() use
    // a data path calculated by the last Init(), so we have to
    // call Init() even if we want to use the compiled-in data
    // path (for this, we pass an empty path to Init()). At the
    // same time, Init() requires at least one language (null
    // language is implicit "eng" in Tesseract versions before 5).
    // As a workaround, we don't check Init() for errors: it will
    // fail if "eng" is unavailable, but the path will still be
    // calculated and stored in TessBaseAPI.

    static std::string result;

    if (result.empty()) {
        ::tesseract::TessBaseAPI tess;
        tess.Init("", nullptr);
        result = tess.GetDatapath();
    }

    return result;
}


std::vector<std::string> getAvailableLangs(std::string_view dataDir)
{
    // We collect language files ourself instead of using
    // TessBaseAPI::GetAvailableLanguagesAsVector():
    //
    // * The signature of the method is different between Tesseract 4
    //   and 5, and we don't want maintain two code paths.
    //
    // * In Tesseract 5.5.0, the method was rewritten to use
    //   std::filesystem under the hood, but due to the lack of any
    //   exception handling, it leaks filesystem_error (e.g. if the
    //   directory does not exist) that we would need to catch.
    //
    //   See https://github.com/tesseract-ocr/tesseract/issues/4364
    //
    // * In Tesseract 5.5.0, the method incorrectly handles file
    //   extensions, only checking if a filename has a ".traineddata"
    //   substring instead of strictly ending with it. As a result,
    //   the list can contain duplicates and entries referring to
    //   ".traineddata***" rather than ".traineddata" files. Removing
    //   those duplicates would require using std::filesystem anyway.
    //
    //   See https://github.com/tesseract-ocr/tesseract/issues/4416

    namespace fs = std::filesystem;

    const auto dataDirPath = fs::u8path(dataDir);

    std::vector<std::string> result;

    std::error_code ec;
    // Like TessBaseAPI::GetAvailableLanguagesAsVector(), we collect
    // languages recursively, but in practice this only makes sense
    // on Unix-like systems when the data dir path is empty (i.e. when
    // using languages from the system package manager), because on
    // certain OS distributions, some languages are actually placed in
    // subdirectories (e.g. "script/").
    for (const auto& entry : fs::recursive_directory_iterator{
            dataDirPath,
            fs::directory_options::skip_permission_denied,
            ec}) {
        if (entry.path().extension() != traineddataExt)
            continue;

        auto lang = entry.path().lexically_relative(dataDirPath)
            .replace_extension().u8string();
        if (!isIgnoredLang(lang))
            result.push_back(std::move(lang));
    }

    if (ec && ec != std::errc::no_such_file_or_directory)
        throw Error{str::format(
            "Directory \"{}\" iterator error: {}",
            dataDir, ec.message())};

    return result;
}


}
