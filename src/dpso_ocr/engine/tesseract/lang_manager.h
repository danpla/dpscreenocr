#pragma once

#include <memory>
#include <string_view>

#include "engine/lang_manager.h"


namespace dpso::ocr::tesseract {


bool hasLangManager();


std::unique_ptr<LangManager> createLangManager(
    std::string_view dataDir,
    std::string_view userAgent,
    std::string_view infoFileUrl);


}
