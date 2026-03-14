// This file is used when the language manager is disabled at compile
// time.

#include "engine/tesseract/lang_manager.h"

#include "engine/lang_manager_error.h"


namespace dpso::ocr::tesseract {


bool hasLangManager()
{
    return false;
}


std::unique_ptr<LangManager> createLangManager(
    std::string_view /*dataDir*/,
    std::string_view /*userAgent*/,
    std::string_view /*infoFileUrl*/)
{
    throw LangManagerError{"Language manager is not available"};
}


}
