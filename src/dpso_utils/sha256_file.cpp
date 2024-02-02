
#include "sha256_file.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <system_error>

#include <fmt/core.h>

#include "os.h"
#include "sha256.h"


namespace dpso {


const char* const sha256FileExt = ".sha256";


std::string calcFileSha256(const char* filePath)
{
    os::StdFileUPtr fp{os::fopen(filePath, "rb")};
    if (!fp)
        throw Sha256FileError{fmt::format(
            "os::fopen(): {}", os::getErrnoMsg(errno))};

    Sha256 h;

    unsigned char buf[32 * 1024];
    while (true) {
        std::size_t numRead{};
        try {
            numRead = os::readSome(fp.get(), buf, sizeof(buf));
        } catch (os::Error& e) {
            throw Sha256FileError{fmt::format(
                "os::readSome(): {}", e.what())};
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

    os::StdFileUPtr fp{os::fopen(sha256FilePath.c_str(), "wb")};
    if (!fp)
        throw Sha256FileError{fmt::format(
            "os::fopen(\"{}\", \"wb\"): {}",
            sha256FilePath, os::getErrnoMsg(errno))};

    try {
        fmt::print(
            fp.get(),
            "{} *{}\n",
            digest,
            os::getBaseName(digestSourceFilePath));
    } catch (std::system_error& e) {
        throw Sha256FileError{fmt::format(
            "fmt::print() to \"{}\": {}", sha256FilePath, e.what())};
    }
}


static std::string loadDigestFromSha256File(
    std::FILE* fp, const char* expectedFileName)
{
    std::string line;

    try {
        os::readLine(fp, line);

        if (std::string extraLine; os::readLine(fp, extraLine))
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

    os::StdFileUPtr fp{os::fopen(sha256FilePath.c_str(), "rb")};
    if (!fp) {
        if (errno == ENOENT)
            return {};

        throw Sha256FileError{fmt::format(
            "os::fopen(\"{}\", \"rb\"): {}",
            sha256FilePath, os::getErrnoMsg(errno))};
    }

    try {
        return loadDigestFromSha256File(
            fp.get(), os::getBaseName(digestSourceFilePath).c_str());
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
