#include "engine/tesseract/utils.h"

#include <string_view>

#include "dpso_utils/str.h"


namespace dpso::ocr::tesseract {


std::size_t prettifyText(char* text)
{
    struct Replacement {
        std::string_view from;
        std::string_view to;
    };

    // Note that ::from must be >= ::to.
    static const Replacement replacements[]{
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

    auto src = str::trim(text, str::isSpace);
    auto* dst = text;

    while (!src.empty()) {
        const auto* prevDst = dst;

        for (const auto& replacement : replacements) {
            if (!str::startsWith(src, replacement.from))
                continue;

            dst += replacement.to.copy(dst, replacement.to.size());
            src.remove_prefix(replacement.from.size());
            break;
        }

        if (dst == prevDst) {
            *dst++ = src.front();
            src.remove_prefix(1);
        }
    }

    *dst = 0;

    return dst - text;
}


}
