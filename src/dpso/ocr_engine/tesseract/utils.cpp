
#include "ocr_engine/tesseract/utils.h"

#include <cassert>
#include <cctype>
#include <cstring>


namespace dpso {


std::size_t prettifyTesseractText(char* text)
{
    struct Replacement {
        const char* from;
        std::size_t fromLen;
        const char* to;
        std::size_t toLen;

        Replacement(const char* from, const char* to)
            : from{from}
            , fromLen{std::strlen(from)}
            , to{to}
            , toLen{std::strlen(to)}
        {
            assert(fromLen >= toLen);
        }
    };

    // Note that ::from must be >= ::to.
    static const Replacement replacements[] = {
        // Tesseract doesn't seem to generate ligatures other than
        // fi and fl.
        {"\357\254\201", "fi"},
        {"\357\254\202", "fl"},
        // If page segmentation is enabled, Tesseract sometimes
        // generates paragraphs consisting of a single space. If such
        // a paragraph is not the first one, the text may look like
        // "paragraph1\n\n \n\nparagraph3". Removing the "\n \n"
        // sequence will result in a normal paragraph separator
        // (empty line). Leading sequences are removed on trimming.
        {"\n \n", ""},
    };

    const auto* src = text;
    auto* dst = text;

    while (std::isspace(*src))
        ++src;

    while (*src) {
        const auto* oldSrc = src;

        for (const auto& replacement : replacements) {
            if (std::strncmp(
                    src,
                    replacement.from,
                    replacement.fromLen) != 0)
                continue;

            std::memmove(dst, replacement.to, replacement.toLen);
            src += replacement.fromLen;
            dst += replacement.toLen;

            break;
        }

        if (src == oldSrc)
            *dst++ = *src++;
    }

    // Tesseract ends text with two newlines, while we need only one.
    if (dst - text >= 2
            && dst[-1] == '\n'
            && dst[-2] == '\n')
        --dst;

    *dst = 0;

    return dst - text;
}


}
