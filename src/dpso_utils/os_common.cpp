#include "os.h"

#include <cstring>
#include <filesystem>
#include <optional>
#include <system_error>

#include "str.h"
#include "stream/file_stream.h"
#include "stream/utils.h"


namespace fs = std::filesystem;


namespace dpso::os {
namespace {


// When wrapping std::filesystem functions, we use error_code instead
// of catching filesystem_error. The latter is merely a wrapper for
// the former, so the only useful information of its what() is the
// text from error_code::message(). Everything else (like paths passed
// to the function, the function name itself, etc.) is unnecessary,
// because the caller already knows this information. For example,
// this is what() for rename() from GCC and Clang, respectively:
//
//   filesystem error: cannot rename: Permission denied [/dev] [/null]
//   filesystem error: in rename: Permission denied ["/dev"] ["/null"]
//
// It also doesn't make sense to catch filesystem_error solely for its
// code() when wrapping a single function: the error_code overloads
// give fewer lines of code because there is no try-catch block.
void check(const char* description, const std::error_code& ec)
{
    if (!ec)
        return;

    const auto message = str::format(
        "{}: {}", description, ec.message());

    if (ec == std::errc::no_such_file_or_directory)
        throw FileNotFoundError{message};

    throw Error{message};
}


}


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


std::string getDirName(const char* path)
{
    return fs::u8path(path).parent_path().u8string();
}


std::string getBaseName(const char* path)
{
    return fs::u8path(path).filename().u8string();
}


std::string getFileExt(const char* filePath)
{
    return fs::u8path(filePath).extension().u8string();
}


std::int64_t getFileSize(const char* filePath)
{
    std::error_code ec;
    const auto result = fs::file_size(fs::u8path(filePath), ec);
    check("fs::file_size", ec);
    return result;
}


void resizeFile(const char* filePath, std::int64_t newSize)
{
    std::error_code ec;
    fs::resize_file(fs::u8path(filePath), newSize, ec);
    check("fs::resize_file", ec);
}


void removeFile(const char* filePath)
{
    std::error_code ec;
    // Note that remove() returns false if the file does not exist. It
    // doesn't set std::errc::no_such_file_or_directory in this case.
    fs::remove(fs::u8path(filePath), ec);
    check("fs::remove", ec);
}


void replace(const char* src, const char* dst)
{
    std::error_code ec;
    fs::rename(fs::u8path(src), fs::u8path(dst), ec);
    check("fs::rename", ec);
}


void makeDirs(const char* dirPath)
{
    std::error_code ec;
    fs::create_directories(fs::u8path(dirPath), ec);
    check("fs::create_directories", ec);
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
