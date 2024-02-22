
#pragma once

#include <cstddef>
#include <string>


namespace dpso {


class File;


class LineReader {
public:
    explicit LineReader(File& file);

    // Read the next line from the file, terminating on either a line
    // break (\r, \n, or \r\n) or the end of the file. Returns false
    // if the line cannot be read because the end of the file is
    // reached.
    //
    // The function clears the line before performing any action.
    //
    // Throws os::Error.
    bool readLine(std::string& line);
private:
    File& file;
    char buf[1024];
    std::size_t bufFill;
    std::size_t bufPos;
};


}
