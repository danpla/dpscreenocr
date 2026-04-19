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

    const auto digest = h.getDigest();
    return str::toHex(digest.data(), digest.size());
}


void saveSha256File(
    std::string_view digestSourceFilePath, std::string_view digest)
{
    validateDigest(digest);

    const auto sha256FilePath =
        std::string{digestSourceFilePath} + sha256FileExt;

    std::optional<FileStream> file;
    try {
        file.emplace(sha256FilePath, FileStream::Mode::write);
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
                os::getBaseName(digestSourceFilePath)));
    } catch (StreamError& e) {
        throw Sha256FileError{str::format(
            "FileStream::write() to \"{}\": {}",
            sha256FilePath, e.what())};
    }
}


namespace {


struct Sha256FileRecord {
    std::string_view digest;
    std::string_view filePath;
};


Sha256FileRecord parseSha256FileLine(std::string_view line)
{
    const auto spacePos = line.find(' ');
    if (spacePos == line.npos)
        throw Sha256FileError{"Digest is not terminated by space"};

    const auto digest = line.substr(0, spacePos);
    validateDigest(digest);

    if (spacePos + 1 == line.size())
        throw Sha256FileError{"No digest mode specifier after space"};

    const auto mode = line[spacePos + 1];
    if (mode != '*')
        throw Sha256FileError{str::format(
            "Expected binary digest mode \"*\", got \"{}\"", mode)};

    const auto filePath = line.substr(spacePos + 2);

    return {digest, filePath};
}


std::string loadDigestFromSha256File(
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

    const auto record = parseSha256FileLine(line);

    if (record.filePath != expectedFileName)
        throw Sha256FileError{str::format(
            "Unexpected file name \"{}\" (should be \"{}\")",
            record.filePath, expectedFileName)};

    return std::string{record.digest};
}


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
            *file, os::getBaseName(digestSourceFilePath));
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
        os::removeFile(sha256FilePath);
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
