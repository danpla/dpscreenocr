
#pragma once


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Get language name.
 *
 * Returns null if the language name for the given code is not known.
 */
const char* dpsoGetLangName(const char* langCode);


#ifdef __cplusplus
}
#endif

