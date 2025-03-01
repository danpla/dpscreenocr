#pragma once

#include <cstdint>
#include <string>


namespace dpso::net {


// Since the function is intended to download relatively small pieces
// of data like JSON responses from various web APIs, it has the
// sizeLimit argument to set a sane size limit on the downloaded data.
// If the response provides the Content-Length header, the limit is
// checked early. At the same time, the function doesn't trust
// Content-Length, and will also check the limit during downloading
// regardless of the presence of the header.
//
// Throws net::Error.
std::string getData(
    const char* url,
    const char* userAgent,
    std::int64_t sizeLimit = 4 * 1024 * 1024);


}
