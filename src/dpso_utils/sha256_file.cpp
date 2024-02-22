
#include "sha256_file.h"

#include <cstring>
#include <optional>

#include <fmt/core.h>

#include "file.h"
#include "line_reader.h"
#include "os.h"
#include "sha256.h"


namespace dpso {


const char* const sha256FileExt = ".sha256";


std::string calcFileSha256(const char* filePath)
{
    std::optional<File> file;
    try {
        file.emplace(filePath, File::Mode::read);
    } catch (os::Error& e) {
        throw Sha256FileError{fmt::format(
            "File(..., Mode::read): {}", e.what())};
    }

    Sha256 h;

    unsigned char buf[32 * 1024];
    while (true) {
        std::size_t numRead{};
        try {
            numRead = file->readSome(buf, sizeof(buf));
        } catch (os::Error& e) {
            throw Sha256FileError{fmt::format(
                "File::readSome(): {}", e.what())};
        }

        h.update(buf, numRead);

        if (numRead < sizeof(buf))
            break;
    }

    return toHex(h.getDigest());
}


void saveSha256File(
    const char* digestSourceFilePath, const char* digest)
{
    const auto sha256FilePath =
        std::string{digestSourceFilePath} + sha256FileExt;

    std::optional<File> file;
    try {
        file.emplace(sha256FilePath.c_str(), File::Mode::write);
    } catch (os::Error& e) {
        throw Sha256FileError{fmt::format(
            "File(\"{}\", Mode::write): {}",
            sha256FilePath, e.what())};
    }

    try {
        write(
            *file,
            fmt::format(
                "{} *{}\n",
                digest,
                os::getBaseName(digestSourceFilePath)));
    } catch (os::Error& e) {
        throw Sha256FileError{fmt::format(
            "File::write() to \"{}\": {}", sha256FilePath, e.what())};
    }
}


static std::string loadDigestFromSha256File(
    File& file, const char* expectedFileName)
{
    std::string line;

    try {
        LineReader lineReader{file};

        lineReader.readLine(line);

        if (std::string extraLine; lineReader.readLine(extraLine))
            throw Sha256FileError{
                "File has more than one line, but only one hash "
                "definition is expected."};
    } catch (os::Error& e) {
        throw Sha256FileError{fmt::format(
            "os::readLine(): {}", e.what())};
    }

    const auto* digestBegin = line.c_str();
    const auto* digestEnd = digestBegin;
    while (*digestEnd && *digestEnd != ' ')
        ++digestEnd;

    const auto digestSize = digestEnd - digestBegin;

    if (digestSize == 0)
        throw Sha256FileError{"Line doesn't start with digest"};

    if (digestSize != Sha256::digestSize * 2)
        throw Sha256FileError{fmt::format(
            "Invalid digest size {} (should be {} for SHA-256)",
            digestSize, Sha256::digestSize * 2)};

    if (*digestEnd != ' ')
        throw Sha256FileError{"Digest is not terminated by space"};

    const auto mode = digestEnd[1];
    if (mode != '*')
        throw Sha256FileError{fmt::format(
            "Expected binary digest mode \"*\", but got \"{}\"",
            mode)};

    const auto* fileName = digestEnd + 2;

    if (std::strcmp(fileName, expectedFileName) != 0)
        throw Sha256FileError{fmt::format(
            "Unexpected file name \"{}\" (should be \"{}\")",
            fileName, expectedFileName)};

    return {digestBegin, digestEnd};
}


std::string loadSha256File(const char* digestSourceFilePath)
{
    const auto sha256FilePath =
        std::string{digestSourceFilePath} + sha256FileExt;

    std::optional<File> file;
    try {
        file.emplace(sha256FilePath.c_str(), File::Mode::read);
    } catch (os::FileNotFoundError&) {
        return {};
    } catch (os::Error& e) {
        throw Sha256FileError{fmt::format(
            "File(\"{}\", Mode::read): {}",
            sha256FilePath, e.what())};
    }

    try {
        return loadDigestFromSha256File(
            *file, os::getBaseName(digestSourceFilePath).c_str());
    } catch (Sha256FileError& e) {
        throw Sha256FileError{fmt::format(
            "\"{}\": {}", sha256FilePath, e.what())};
    }
}


void removeSha256File(const char* digestSourceFilePath)
{
    const auto sha256FilePath =
        std::string{digestSourceFilePath} + sha256FileExt;

    try {
        os::removeFile(sha256FilePath.c_str());
    } catch (os::FileNotFoundError&) {
    } catch (os::Error& e) {
        throw Sha256FileError{fmt::format(
            "os::removeFile(\"{}\"): {}", sha256FilePath, e.what())};
    }
}


std::string getSha256HexDigestWithCaching(const char* filePath)
{
    std::string digest;

    try {
        digest = loadSha256File(filePath);
    } catch (Sha256FileError& e) {
        throw Sha256FileError{fmt::format(
            "Can't load digest: {}", e.what())};
    }

    if (!digest.empty())
        return digest;

    try {
        digest = calcFileSha256(filePath);
    } catch (Sha256FileError& e) {
        throw Sha256FileError{fmt::format(
            "Can't calculate digest: {}", e.what())};
    }

    try {
        saveSha256File(filePath, digest.c_str());
    } catch (Sha256FileError& e) {
        throw Sha256FileError{fmt::format(
            "Can't save digest: {}", e.what())};
    }

    return digest;
}


}
