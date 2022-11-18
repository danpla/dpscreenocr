
#include "strftime.h"


namespace dpso {


std::string strftime(const char* format, const std::tm* time)
{
    // strftime() does not set errno if the destination buffer is too
    // small, making it impossible to distinguish this error from
    // cases when the result string is empty (for example, %p yields
    // an empty string in many locales). As a workaround, we append an
    // extra character to the format string, and then remove it from
    // the result before returning.
    const auto formatEx = std::string{format} + ' ';

    std::string result;

    while (true) {
        result.reserve(result.size() + 8);
        result.resize(result.capacity());

        const auto numWritten = std::strftime(
            &result[0], result.size(), formatEx.c_str(), time);
        if (numWritten == 0)
            continue;

        result.resize(numWritten - 1);
        break;
    }

    return result;
}


}
