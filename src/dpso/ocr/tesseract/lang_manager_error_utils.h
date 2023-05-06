
#pragma once


namespace dpso::ocr::tesseract {


// This function is intended to be called from a net::Error catch
// clause.
//
// Keep in mind that the message is the only text to be included in
// the exception; you are expected to create the full text based on
// net::Error::what() before calling this function.
[[noreturn]]
void rethrowNetErrorAsLangManagerError(const char* message);


}
