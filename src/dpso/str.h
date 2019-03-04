
#pragma once

#include <cstddef>


namespace dpso {
namespace str {


/**
 * Compare str with at most subStrLen characters of subStr.
 *
 * The function allows to compare a string with a part of another
 * string without the need to null-terminate the part manually.
 *
 * It's similar to strncmp(), except that if first subStr characters
 * are equal and no null was found, the function additionally checks
 * whether str[subStrLen] is null to ensure that subStr is not a
 * prefix of str.
 *
 * For example ("Foo", "FooBar", 3) == 0, but ("FooBar", "Foo", 3)
 * > 0.
 */
int cmpSubStr(
    const char* str,
    const char* subStr, std::size_t subStrLen = -1,
    bool ignoreCase = false);


/**
 * Compare two strings ignoring case.
 */
inline int cmpIc(const char* a, const char* b)
{
    return cmpSubStr(a, b, -1, true);
}


/**
 * Prettify text returned by Tesseract.
 *
 * This function:
 *
 *   * Strips leading whitespace.
 *   * Splits fi and fl ligatures.
 *   * Removes paragraphs consisting of a single space, which are
 *       sometimes created when page segmentation is enabled.
 *   * Removes the trailing newline (Tesseract adds two).
 *
 * newLen, if not null, is set to a new text len.
 */
void prettifyOcrText(char* text, std::size_t* newLen = nullptr);


}
}
