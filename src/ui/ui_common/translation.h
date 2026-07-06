#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Get the code of the currently active translation language.
 *
 * Language codes match the naming convention used for the message
 * catalogs (MO files), and consist of one or more alphanumeric
 * strings separated by underscores. Examples of language codes
 * include "en" (English), "en_GB" (British English) and "zh_Hans"
 * (Simplified Chinese).
 *
 * The default is "en" (English), which matches the language used in
 * the source strings passed to uiTranslate*() functions. The returned
 * string pointer remains valid till the next call to
 * uiPickTranslationLang().
 */
const char* uiGetTranslationLang(void);


/**
 * Translate a message.
 *
 * The method searches the message catalog (MO file) for a message
 * with the given ID. If the message is not present in the catalog
 * (i.e., it has not been translated), the ID itself is returned.
 */
const char* uiTranslate(const char* id);


/**
 * Translate a message using a context to resolve ambiguities.
 *
 * The method is similar to translate(), but takes an additional
 * context string to resolve ambiguities in cases when the same
 * message ID can have different meanings depending on the
 * context.
 */
const char* uiTranslateContext(const char* context, const char* id);


/**
 * Translate a plural form message.
 *
 * The n parameter is an arbitrary number used to select the plural
 * form based on the rules of the language.
 *
 * The method searches the message catalog (MO file) for a message
 * with the ID equal to the idSingular argument. If the message is
 * found, the plural rule from the catalog is used to select the
 * translated plural form. If the message is not present in the
 * catalog (i.e., it has not been translated), then idSingular is
 * returned if n == 1, and idPlural is returned in all other cases.
 */
const char* uiTranslateN(
    uint64_t n, const char* idSingular, const char* idPlural);


#ifdef __cplusplus
}
#endif
