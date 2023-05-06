
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
    DownloadProgressHandler progressHandler)
{
    const auto downloadPartPath = std::string{filePath} + ".part";

    StdFileUPtr downloadPart{
        dpsoFopen(downloadPartPath.c_str(), "wb")};
    if (!downloadPart)
        throw Error{str::printf(
            "Can't open \"%s\": %s",
            downloadPartPath.c_str(),
            std::strerror(errno))};

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

        if (std::fwrite(buf, 1, numRead, downloadPart.get())
                != numRead)
            throw Error{str::printf(
                "fwrite() to \"%s\" failed",
                downloadPartPath.c_str())};

        if (!progressHandler)
            continue;

        const auto curTime = Clock::now();
        if (curTime - lastProgressReportTime < progressReportInterval)
            continue;

        lastProgressReportTime = curTime;

        const auto curSize = std::ftell(downloadPart.get());

        if (!progressHandler(curSize, fileSize)) {
            downloadPart.reset();
            dpsoRemove(downloadPartPath.c_str());  // Ignore errors
            return;
        }
    }

    if (std::fflush(downloadPart.get()) == EOF)
        throw Error{str::printf(
            "fflush() for \"%s\" failed", downloadPartPath.c_str())};

    if (!dpsoSyncFile(downloadPart.get()))
        throw Error{str::printf(
            "dpsoSyncFile() for \"%s\": %s",
            downloadPartPath.c_str(),
            dpsoGetError())};

    downloadPart.reset();

    if (!dpsoReplace(downloadPartPath.c_str(), filePath))
        throw Error{str::printf(
            "dpsoReplace(\"%s\", \"%s\"): %s",
            downloadPartPath.c_str(),
            filePath,
            dpsoGetError())};
}


}
