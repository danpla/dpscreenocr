
#include "os.h"

#include <cstring>


namespace dpso::os {


const char* getFileExt(const char* filePath)
{
    const char* ext{};

    for (const auto* s = filePath; *s; ++s)
        if (*s == '.')
            ext = s;
        else if (std::strchr(dirSeparators, *s))
            ext = nullptr;

    if (ext
            && ext[1]
            // A leading period denotes a "hidden" file on Unix-like
            // systems. We follow this convention on all platforms.
            && ext != filePath
            && !std::strchr(dirSeparators, ext[-1]))
        return ext;

    return nullptr;
}


bool readLine(std::FILE* fp, std::string& line)
{
    line.clear();

    while (true) {
        const auto c = std::fgetc(fp);
        if (c == EOF)
            return !std::ferror(fp) && !line.empty();

        if (c == '\r' || c == '\n') {
            if (c == '\r')
                if (const auto c2 = std::fgetc(fp); c2 != '\n')
                    std::ungetc(c2, fp);

            break;
        }

        line += c;
    }

    return true;
}


}
