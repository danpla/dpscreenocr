#include "sha256_file.h"

#include <optional>

#include "line_reader.h"
#include "os.h"
#include "sha256.h"
#include "str.h"
#include "stream/file_stream.h"
#include "stream/utils.h"


namespace dpso {


const std::string sha256FileExt{".sha256"};


static void validateDigest(std::string_view digest)
{
    const auto sha256HexDigestSize = Sha256::digestSize * 2;

    if (digest.size() != sha256HexDigestSize)
        throw Sha256FileError{str::format(
            "Invalid hex digest size {} (should be {} for SHA-256)",
            digest.size(), sha256HexDigestSize)};
}


std::string calcFileSha256(std::string_view filePath)
{
    std::optional<FileStream> file;
    try {
        file.emplace(filePath, FileStream::Mode::read);
    } catch (os::Error& e) {
        throw Sha256FileError{str::format(
            "FileStream(..., Mode::read): {}", e.what())};
    }

    Sha256 h;

    unsigned char buf[32 * 1024];
    while (true) {
        std::size_t numRead{};
        try {
            numRead = file->readSome(buf, sizeof(buf));
        } catch (StreamError& e) {
            throw Sha256FileError{str::format(
                "FileStream::readSome(): {}", e.what())};
        }

        h.update(buf, numRead);

        if (numRead < sizeof(buf))
            break;
    }

    return toHex(h.getDigest());
}


void saveSha256File(
    std::string_view digestSourceFilePath, std::string_view digest)
{
    validateDigest(digest);

    const auto sha256FilePath =
        std::string{digestSourceFilePath} + sha256FileExt;

    std::optional<FileStream> file;
    try {
        file.emplace(sha256FilePath.c_str(), FileStream::Mode::write);
    } catch (os::Error& e) {
        throw Sha256FileError{str::format(
            "FileStream(\"{}\", Mode::write): {}",
            sha256FilePath, e.what())};
    }

    try {
        write(
            *file,
            str::format(
                "{} *{}\n",
                digest,
                os::getBaseName(
                    std::string{digestSourceFilePath}.c_str())));
    } catch (StreamError& e) {
        throw Sha256FileError{str::format(
            "FileStream::write() to \"{}\": {}",
            sha256FilePath, e.what())};
    }
}


static std::string loadDigestFromSha256File(
    Stream& stream, std::string_view expectedFileName)
{
    std::string line;

    try {
        LineReader lineReader{stream};

        lineReader.readLine(line);

        if (std::string extraLine; lineReader.readLine(extraLine))
            throw Sha256FileError{
                "File has more than one line, but only one hash "
                "definition is expected."};
    } catch (StreamError& e) {
        throw Sha256FileError{str::format(
            "os::readLine(): {}", e.what())};
    }

    const auto spacePos = line.find(' ');
    if (spacePos == line.npos)
        throw Sha256FileError{"Digest is not terminated by space"};

    const std::string_view digest{line.data(), spacePos};
    validateDigest(digest);

    if (spacePos + 1 == line.size())
        throw Sha256FileError{"No digest mode specifier after space"};

    const auto mode = line[spacePos + 1];
    if (mode != '*')
        throw Sha256FileError{str::format(
            "Expected binary digest mode \"*\", but got \"{}\"",
            mode)};

    const auto fileName = std::string_view{line}.substr(spacePos + 2);
    if (fileName != expectedFileName)
        throw Sha256FileError{str::format(
            "Unexpected file name \"{}\" (should be \"{}\")",
            fileName, expectedFileName)};

    return std::string{digest};
}


std::string loadSha256File(std::string_view digestSourceFilePath)
{
    const auto sha256FilePath =
        std::string{digestSourceFilePath} + sha256FileExt;

    std::optional<FileStream> file;
    try {
        file.emplace(sha256FilePath, FileStream::Mode::read);
    } catch (os::FileNotFoundError&) {
        return {};
    } catch (os::Error& e) {
        throw Sha256FileError{str::format(
            "FileStream(\"{}\", Mode::read): {}",
            sha256FilePath, e.what())};
    }

    try {
        return loadDigestFromSha256File(
            *file,
            os::getBaseName(
                std::string{digestSourceFilePath}.c_str()));
    } catch (Sha256FileError& e) {
        throw Sha256FileError{str::format(
            "\"{}\": {}", sha256FilePath, e.what())};
    }
}


void removeSha256File(std::string_view digestSourceFilePath)
{
    const auto sha256FilePath =
        std::string{digestSourceFilePath} + sha256FileExt;

    try {
        os::removeFile(sha256FilePath.c_str());
    } catch (os::Error& e) {
        throw Sha256FileError{str::format(
            "os::removeFile(\"{}\"): {}", sha256FilePath, e.what())};
    }
}


std::string getSha256HexDigestWithCaching(std::string_view filePath)
{
    std::string digest;

    try {
        digest = loadSha256File(filePath);
    } catch (Sha256FileError& e) {
        throw Sha256FileError{str::format(
            "Can't load digest: {}", e.what())};
    }

    if (!digest.empty())
        return digest;

    try {
        digest = calcFileSha256(filePath);
    } catch (Sha256FileError& e) {
        throw Sha256FileError{str::format(
            "Can't calculate digest: {}", e.what())};
    }

    try {
        saveSha256File(filePath, digest);
    } catch (Sha256FileError& e) {
        throw Sha256FileError{str::format(
            "Can't save digest: {}", e.what())};
    }

    return digest;
}


}
