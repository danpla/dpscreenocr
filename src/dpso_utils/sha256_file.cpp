
#include "sha256_file.h"

#include <cerrno>
#include <cstdio>
#include <cstring>

#include "os.h"
#include "sha256.h"
#include "str.h"


namespace dpso {


const char* const sha256FileExt = ".sha256";


std::string calcFileSha256(const char* filePath)
{
    StdFileUPtr fp{std::fopen(filePath, "rb")};
    if (!fp)
        throw Sha256FileError{str::printf(
            "fopen: %s", std::strerror(errno))};

    Sha256 h;

    unsigned char buf[32 * 1024];
    while (true) {
        const auto numRead = std::fread(
            buf, 1, sizeof(buf), fp.get());

        const auto isLastRead = numRead < sizeof(buf);
        if (isLastRead && std::ferror(fp.get()))
            throw Sha256FileError{str::printf(
                "fread: %s", std::strerror(errno))};

        h.update(buf, numRead);

        if (isLastRead)
            break;
    }

    return toHex(h.getDigest());
}


static const char* getFileName(const char* path)
{
    const auto* result = path;

    for (const auto* s = path; *s; ++s)
        if (std::strchr(dpsoDirSeparators, *s))
            result = s + 1;

    return result;
}


void saveSha256File(
    const char* digestSourceFilePath, const char* digest)
{
    const auto sha256FilePath =
        std::string{digestSourceFilePath} + sha256FileExt;

    StdFileUPtr fp{std::fopen(sha256FilePath.c_str(), "wb")};
    if (!fp)
        throw Sha256FileError{str::printf(
            "fopen(\"%s\", \"wb\"): %s",
            sha256FilePath.c_str(), std::strerror(errno))};

    if (std::fprintf(
            fp.get(),
            "%s *%s\n",
            digest,
            getFileName(digestSourceFilePath)) < 0)
        throw Sha256FileError{str::printf(
            "fprintf to \"%s\" failed", sha256FilePath.c_str())};
}


static std::string loadDigestFromSha256File(
    std::FILE* fp, const char* expectedFileName)
{
    std::string line;

    while (true) {
        const auto c = std::fgetc(fp);
        if (c == EOF || c == '\r' || c == '\n') {
            if (c == '\r')
                if (const auto c2 = std::fgetc(fp); c2 != '\n')
                    std::ungetc(c2, fp);

            break;
        }

        line += c;
    }

    if (std::fgetc(fp) != EOF)
        throw Sha256FileError{
            "File has more than one line, but only one hash "
            "definition is expected."};

    if (std::ferror(fp))
        throw Sha256FileError{"Read error"};

    const auto* digestBegin = line.c_str();
    const auto* digestEnd = digestBegin;
    while (*digestEnd && *digestEnd != ' ')
        ++digestEnd;

    const auto digestSize = digestEnd - digestBegin;

    if (digestSize == 0)
        throw Sha256FileError{"Line doesn't start with digest"};

    if (digestSize != Sha256::digestSize * 2)
        throw Sha256FileError{str::printf(
            "Invalid digest size %zu (should be %zu for SHA-256)",
            digestSize, Sha256::digestSize * 2)};

    if (*digestEnd != ' ')
        throw Sha256FileError{"Digest is not terminated by space"};

    const auto mode = digestEnd[1];
    if (mode != '*')
        throw Sha256FileError{str::printf(
            "Expected binary digest mode \"*\", but got \"%c\"",
            mode)};

    const auto* fileName = digestEnd + 2;

    if (std::strcmp(fileName, expectedFileName) != 0)
        throw Sha256FileError{str::printf(
            "Unexpected file name \"%s\" (should be \"%s\")",
            fileName, expectedFileName)};

    return {digestBegin, digestEnd};
}


std::string loadSha256File(const char* digestSourceFilePath)
{
    const auto sha256FilePath =
        std::string{digestSourceFilePath} + sha256FileExt;

    StdFileUPtr fp{std::fopen(sha256FilePath.c_str(), "rb")};
    if (!fp) {
        if (errno == ENOENT)
            return {};

        throw Sha256FileError{str::printf(
            "fopen(\"%s\", \"rb\"): %s",
            sha256FilePath.c_str(), std::strerror(errno))};
    }

    try {
        return loadDigestFromSha256File(
            fp.get(), getFileName(digestSourceFilePath));
    } catch (Sha256FileError& e) {
        throw Sha256FileError{str::printf(
            "\"%s\": %s",
            sha256FilePath.c_str(), e.what())};
    }
}


void removeSha256File(const char* digestSourceFilePath)
{
    const auto sha256FilePath =
        std::string{digestSourceFilePath} + sha256FileExt;

    if (dpsoRemove(sha256FilePath.c_str()) != 0 && errno != ENOENT)
        throw Sha256FileError{str::printf(
            "dpsoRemove(\"%s\"): %s",
            sha256FilePath.c_str(), std::strerror(errno))};
}


std::string getSha256HexDigestWithCaching(const char* filePath)
{
    std::string digest;

    try {
        digest = loadSha256File(filePath);
    } catch (Sha256FileError& e) {
        throw Sha256FileError{str::printf(
            "Can't load digest: %s", e.what())};
    }

    if (!digest.empty())
        return digest;

    try {
        digest = calcFileSha256(filePath);
    } catch (Sha256FileError& e) {
        throw Sha256FileError{str::printf(
            "Can't calculate digest: %s", e.what())};
    }

    try {
        saveSha256File(filePath, digest.c_str());
    } catch (Sha256FileError& e) {
        throw Sha256FileError{str::printf(
            "Can't save digest: %s", e.what())};
    }

    return digest;
}


}
