
#pragma once

#include <string>


namespace test {
namespace utils {


// Escape non-printable characters with C-style sequences.
std::string escapeStr(const char* str);


// The toStr() family of functions are the helpers intended to format
// variables for test messages. The variants taking a single argument
// are mostly meant for the toStr() variant taking two iterators to
// format a range of values, but can also be used individually. You
// can provide your own overloads of toStr() for custom types so that
// the range-based toStr() accepts them.


// Returns "true" or "false".
std::string toStr(bool b);


// toStr() overloads for C string and std::string format a string as a
// C string literal: it's passed though escapeStr() and enclosed in
// double quotes. The C string variant also gives "nullptr" for null.
std::string toStr(const char* str);
std::string toStr(const std::string& str);


template<typename T>
std::string toStr(T begin, T end)
{
    std::string result = "{";

    for (auto iter = begin; iter != end; ++iter) {
        if (iter != begin)
            result += ", ";

        result += toStr(*iter);
    }

    result += '}';

    return result;
}


// Convert all line feeds (\n) to native line endings for the current
// platform. This function uses the same line endings as written by
// fopen() without the "b" flag.
std::string lfToNativeNewline(const char* str);


// In failure, calls tests::fatalError() using contextInfo as a part
// of the error message.
void saveText(
    const char* contextInfo, const char* filePath, const char* text);


// In failure, calls tests::fatalError() using contextInfo as a part
// of the error message.
std::string loadText(const char* contextInfo, const char* filePath);


// Print the first difference between two possibly multiline texts.
// Non-printable characters in each printed line are escaped as by
// escapeStr(). Does nothing if texts are equal.
void printFirstDifference(const char* expected, const char* actual);


}
}
