
#pragma once

#include <cstddef>


namespace dpso::ocr::tesseract {


// Prettify text returned by Tesseract.
//
// The function will:
// * Trim whitespace.
// * Split fi and fl ligatures.
// * Remove paragraphs consisting of a single space, which are
//   sometimes created when page segmentation is enabled.
//
// Returns the new length of the text.
std::size_t prettifyText(char* text);


}
