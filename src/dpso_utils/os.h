
#pragma once

#include <cstdio>


namespace dpso {


/**
 * Directory separators for the current platform.
 *
 * The preferred separator is first in the list.
 */
extern const char* const dirSeparators;


std::FILE* fopenUtf8(const char* fileName, const char* mode);


}
