
#pragma once

#include <cstddef>

#include "types.h"


namespace dpso {


const char* keyToString(DpsoKey key);
DpsoKey keyFromString(const char* str, std::size_t strLen = -1);

const char* modToString(DpsoKeyMod mod);
DpsoKeyMod modFromString(const char* str, std::size_t strLen = -1);


}
