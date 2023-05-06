
#pragma once

#include <cstdint>
#include <string>
#include <vector>


namespace dpso::ocr::tesseract {


struct ExternalLangInfo {
    std::string code;
    std::int64_t size;
    std::string url;
};


std::vector<ExternalLangInfo> getExternalLangs(const char* userAgent);


}
