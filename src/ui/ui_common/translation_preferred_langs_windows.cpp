#include "translation_preferred_langs.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "dpso_utils/windows/utf.h"


namespace ui {


std::vector<std::string> getPreferredTranslationLangs()
{
    const DWORD flags = MUI_LANGUAGE_NAME;

    ULONG numLangs{};
    ULONG bufSize{};
    if (!GetUserPreferredUILanguages(
            flags, &numLangs, nullptr, &bufSize))
        return {};

    std::wstring buf(bufSize, 0);
    if (!GetUserPreferredUILanguages(
            flags, &numLangs, buf.data(), &bufSize))
        return {};

    std::vector<std::string> result;
    result.reserve(numLangs);

    for (const auto* s = buf.data(); numLangs--;) {
        try {
            result.push_back(dpso::windows::utf16ToUtf8(s));
        } catch (dpso::windows::CharConversionError&) {
            break;
        }

        // Skip the added language and its null terminator.
        s += result.back().size() + 1;
    }

    return result;
}


}
