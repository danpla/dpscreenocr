#pragma once

#include <cstddef>
#include <string>


namespace dpso {


class Stream;


class LineReader {
public:
    explicit LineReader(Stream& stream);

    // Read the next line from the stream, terminating on either a
    // line break (\r, \n, or \r\n) or the end of the stream. Returns
    // false if the line cannot be read because the end of the stream
    // is reached.
    //
    // The function clears the line before performing any action.
    //
    // Throws StreamError.
    bool readLine(std::string& line);
private:
    Stream& stream;
    char buf[1024];
    std::size_t bufFill;
    std::size_t bufPos;
};


}
