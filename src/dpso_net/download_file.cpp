
#include "download_file.h"

#include <chrono>
#include <string>

#include "dpso_utils/os.h"
#include "dpso_utils/str.h"
#include "dpso_utils/stream/file_stream.h"

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

    std::optional<FileStream> partFile;
    try {
        partFile.emplace(partPath.c_str(), FileStream::Mode::write);
    } catch (os::Error& e) {
        throw Error{str::format(
            "FileStream(\"{}\", Mode::write): {}",
            partPath, e.what())};
    }

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
            partFile->write(buf, numRead);
        } catch (StreamError& e) {
            throw Error{str::format(
                "FileStream::write() to \"{}\": {}",
                partPath, e.what())};
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
            partFile.reset();

            try {
                os::removeFile(partPath.c_str());
            } catch (os::Error&) {
            }

            return;
        }
    }

    try {
        partFile->sync();
    } catch (os::Error& e) {
        throw Error{str::format(
            "FileStream::sync() for \"{}\": {}", partPath, e.what())};
    }

    partFile.reset();

    try {
        os::replace(partPath.c_str(), filePath);
    } catch (os::Error& e) {
        throw Error{str::format(
            "os::replace(\"{}\", \"{}\"): {}",
            partPath, filePath, e.what())};
    }
}


}
