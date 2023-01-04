
#pragma once

#include <string>


namespace test {
namespace utils {


std::string boolToStr(bool b);


// Escape non-printable characters with C-style sequences.
std::string escapeStr(const char* str);


// Convert all line feeds (\n) to native line endings for the current
// platform. This function uses the same line endings as written by
// fopen() without the "b" flag.
std::string lfToNativeNewline(const char* str);


// In failure, calls tests::fatalError() using contextInfo as a part
// of the error message.
std::string loadFileText(
    const char* contextInfo, const char* filePath);


// Print the first difference between two possibly multiline texts.
// Non-printable characters in each printed line are escaped as by
// escapeStr(). Does nothing if texts are equal.
void printFirstDifference(const char* expected, const char* actual);


}
}
