
#pragma once

#include <cstddef>
#include <string>


namespace dpso {


class Stream;


// Read the given number of bytes from a stream. Throws StreamError.
void read(Stream& stream, void* dst, std::size_t dstSize);


// Write data to a stream. Throws StreamError.
void write(Stream& stream, const std::string& str);
void write(Stream& stream, const char* str);
void write(Stream& stream, char c);


}
