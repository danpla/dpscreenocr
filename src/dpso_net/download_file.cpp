
#include "download_file.h"

#include <cerrno>
#include <chrono>
#include <string>

#include <fmt/core.h>

#include "dpso_utils/os.h"

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

    os::StdFileUPtr partFp{os::fopen(partPath.c_str(), "wb")};
    if (!partFp)
        throw Error{fmt::format(
            "Can't open \"{}\": {}",
            partPath, os::getErrnoMsg(errno))};

    auto response = makeGetRequest(url, userAgent);

    const auto fileSize = response->getSize();
    std::int64_t partSize{};

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

        try {
            os::write(partFp.get(), buf, numRead);
        } catch (os::Error& e) {
            throw Error{fmt::format(
                "os::write() to \"{}\": {}", partPath, e.what())};
        }

        partSize += numRead;

        if (!progressHandler)
            continue;

        const auto curTime = Clock::now();
        if (lastProgressReportTime != Clock::time_point{}
                && curTime - lastProgressReportTime
                    < progressReportInterval)
            continue;

        lastProgressReportTime = curTime;

        if (!progressHandler(partSize, fileSize)) {
            partFp.reset();

            try {
                os::removeFile(partPath.c_str());
            } catch (os::Error&) {
            }

            return;
        }
    }

    if (std::fflush(partFp.get()) == EOF)
        throw Error{fmt::format(
            "fflush() for \"{}\" failed", partPath)};

    try {
        os::syncFile(partFp.get());
    } catch (os::Error& e) {
        throw Error{fmt::format(
            "os::syncFile() for \"{}\": {}", partPath, e.what())};
    }

    partFp.reset();

    try {
        os::replace(partPath.c_str(), filePath);
    } catch (os::Error& e) {
        throw Error{fmt::format(
            "os::replace(\"{}\", \"{}\"): {}",
            partPath, filePath, e.what())};
    }
}


}
