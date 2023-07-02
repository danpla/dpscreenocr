
#pragma once

#include <cstdint>
#include <functional>
#include <optional>


namespace dpso::net {


// If the file size is unknown (e.g. the server does not provide
// the Content-Length header), the function is called with nullopt
// totalSize.
//
// Returns false to terminate downloading and remove a partially
// downloaded file.
using DownloadProgressHandler = std::function<
    bool(
        std::int64_t curSize,
        std::optional<std::int64_t> totalSize)>;


// The function will not download the file directly to filePath, but
// instead will use a temporary file named as filePath, but with an
// additional extension. Once the temporary file is downloaded, it's
// renamed to filePath, silently rewriting an existing file, if any.
//
// Throws net::Error.
void downloadFile(
    const char* url,
    const char* userAgent,
    const char* filePath,
    const DownloadProgressHandler& progressHandler);


}
