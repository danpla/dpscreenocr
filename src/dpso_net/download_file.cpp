
#include "download_file.h"

#include <cerrno>
#include <chrono>
#include <cstring>
#include <string>

#include "dpso_utils/error.h"
#include "dpso_utils/os.h"
#include "dpso_utils/str.h"

#include "error.h"
#include "request.h"


namespace dpso::net {


// TODO: Continue interrupted downloads with range requests:
// https://developer.mozilla.org/en-US/docs/Web/HTTP/Range_requests


void downloadFile(
    const char* url,
    const char* userAgent,
    const char* filePath,
    const DownloadProgressHandler& progressHandler)
{
    const auto partPath = std::string{filePath} + ".part";

    StdFileUPtr partFp{dpsoFopen(partPath.c_str(), "wb")};
    if (!partFp)
        throw Error{str::printf(
            "Can't open \"%s\": %s",
            partPath.c_str(), std::strerror(errno))};

    auto response = makeGetRequest(url, userAgent);

    const auto fileSize = response->getSize();

    using Clock = std::chrono::steady_clock;
    Clock::time_point lastProgressReportTime;

    // There's no sense to report the progress more often than an
    // average monitor refresh rate (60 Hz).
    const std::chrono::milliseconds progressReportInterval{1000 / 60};

    unsigned char buf[16 * 1024];
    while (true) {
        const auto numRead = response->read(buf, sizeof(buf));
        if (numRead == 0)
            break;

        if (std::fwrite(buf, 1, numRead, partFp.get()) != numRead)
            throw Error{str::printf(
                "fwrite() to \"%s\" failed", partPath.c_str())};

        if (!progressHandler)
            continue;

        const auto curTime = Clock::now();
        if (lastProgressReportTime != Clock::time_point{}
                && curTime - lastProgressReportTime
                    < progressReportInterval)
            continue;

        lastProgressReportTime = curTime;

        const auto curSize = std::ftell(partFp.get());

        if (!progressHandler(curSize, fileSize)) {
            partFp.reset();
            dpsoRemove(partPath.c_str());  // Ignore errors
            return;
        }
    }

    if (std::fflush(partFp.get()) == EOF)
        throw Error{str::printf(
            "fflush() for \"%s\" failed", partPath.c_str())};

    if (!dpsoSyncFile(partFp.get()))
        throw Error{str::printf(
            "dpsoSyncFile() for \"%s\": %s",
            partPath.c_str(), dpsoGetError())};

    partFp.reset();

    if (!dpsoReplace(partPath.c_str(), filePath))
        throw Error{str::printf(
            "dpsoReplace(\"%s\", \"%s\"): %s",
            partPath.c_str(), filePath, dpsoGetError())};
}


}
