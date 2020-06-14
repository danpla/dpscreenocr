
#pragma once

#include <cstddef>


namespace dpso {


/**
 * Prettify text returned by Tesseract.
 *
 * The function:
 *   * Strips leading whitespace.
 *   * Splits fi and fl ligatures.
 *   * Removes paragraphs consisting of a single space, which are
 *       sometimes created when page segmentation is enabled.
 *   * Removes the trailing newline (Tesseract adds two).
 *
 * Returns new length of the text (<= the original).
 */
std::size_t prettifyTesseractText(char* text);


}
