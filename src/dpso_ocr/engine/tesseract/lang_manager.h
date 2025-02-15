#pragma once

#include <memory>

#include "engine/lang_manager.h"


namespace dpso::ocr::tesseract {


bool hasLangManager();


std::unique_ptr<LangManager> createLangManager(
    const char* dataDir,
    const char* userAgent,
    const char* infoFileUrl);


}
