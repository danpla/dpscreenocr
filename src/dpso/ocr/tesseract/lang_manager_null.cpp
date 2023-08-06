
// This file is used when the language manager is disabled at compile
// time.

#include "ocr/tesseract/lang_manager.h"

#include "ocr/lang_manager_error.h"


namespace dpso::ocr::tesseract {


bool hasLangManager()
{
    return false;
}


std::unique_ptr<LangManager> createLangManager(
    const char* dataDir,
    const char* userAgent,
    const char* infoFileUrl)
{
    (void)dataDir;
    (void)userAgent;
    (void)infoFileUrl;

    throw LangManagerError{"Language manager is not available"};
}


}
