
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>


namespace dpso::net {


class Response {
public:
    virtual ~Response() = default;

    // Returns size of response data. This is a cached value of the
    // Content-Length header, if any.
    virtual std::optional<std::int64_t> getSize() const = 0;

    // Read up to dstSize bytes from the response. Returns 0 if the
    // end is reached. Throws net::Error on error.
    virtual std::size_t read(void* dst, std::size_t dstSize) = 0;
};


// Make HTTP GET request.
//
// Throws net::Error on error, or on any HTTP response code other than
// 200.
std::unique_ptr<Response> makeGetRequest(
    const char* url, const char* userAgent);


}
