#pragma once

#include <vector>


namespace dpso::sound {


struct FormatInfo {
    const char* name;
    // The list of possible file extensions for the format. Each
    // extension has a leading period.
    std::vector<const char*> extensions;
};


}
