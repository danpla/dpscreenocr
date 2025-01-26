#include "os.h"

#include <cstring>
#include <optional>

#include "str.h"
#include "stream/file_stream.h"
#include "stream/utils.h"


namespace dpso::os {


[[noreturn]]
void throwErrno(const char* description, int errnum)
{
    const auto message = str::format(
        "{}: {}", description, getErrnoMsg(errnum));

    switch (errnum) {
    case ENOENT:
        throw FileNotFoundError{message};
    default:
        throw Error{message};
    }
}


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


std::string loadData(const char* filePath)
{
    std::int64_t fileSize;
    try {
        fileSize = getFileSize(filePath);
    } catch (FileNotFoundError&) {
        throw;
    } catch (Error& e) {
        throw Error{str::format("os::getFileSize(): {}", e.what())};
    }

    std::optional<FileStream> file;
    try {
        file.emplace(filePath, FileStream::Mode::read);
    } catch (FileNotFoundError&) {
        throw;
    } catch (Error& e) {
        throw Error{str::format(
            "FileStream(..., Mode::read): {}", e.what())};
    }

    std::string result;
    result.resize(fileSize);

    try {
        read(*file, result.data(), result.size());
    } catch (StreamError& e) {
        throw Error{str::format("read(file, ...): {}", e.what())};
    }

    return result;
}


}
