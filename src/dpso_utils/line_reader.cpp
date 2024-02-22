
#include "line_reader.h"

#include <cassert>

#include "file.h"


namespace dpso {


LineReader::LineReader(File& file)
    : file{file}
    , bufFill{}
    , bufPos{}
{
}


bool LineReader::readLine(std::string& line)
{
    line.clear();

    auto wasCr = bufPos > 0 && buf[bufPos - 1] == '\r';

    while (true) {
        assert(bufPos <= bufFill);

        if (bufPos == bufFill) {
            bufFill = file.readSome(buf, sizeof(buf));
            bufPos = 0;

            if (bufFill == 0)
                break;
        }

        if (wasCr) {
            wasCr = false;
            if (buf[bufPos] == '\n') {
                ++bufPos;
                continue;
            }
        }

        auto partEnd = bufPos;
        for (; partEnd < bufFill; ++partEnd)
            if (buf[partEnd] == '\r' || buf[partEnd] == '\n')
                break;

        line.append(buf + bufPos, buf + partEnd);
        bufPos = partEnd;

        if (bufPos < bufFill) {
            ++bufPos;
            break;
        }
    }

    return !line.empty() || bufFill > 0;
}


}
