#include "request.h"

#include <algorithm>
#include <cassert>
#include <charconv>
#include <string>

#include <curl/curl.h>

#include "dpso_utils/str.h"

#include "error.h"
#include "request_curl_lib.h"
#include "request_curl_lib_utils.h"


namespace dpso::net {
namespace {


const CurlGlobalInit curlGlobalInit;


class CurlResponse : public Response {
public:
    CurlResponse(std::string_view url, std::string_view userAgent);

    std::optional<std::int64_t> getSize() const override
    {
        return contentLength;
    }

    std::size_t read(void* dst, std::size_t dstSize) override;
private:
    const LibCurl& libCurl;
    char curlError[CURL_ERROR_SIZE]{};
    CurlMUPtr curlM;
    CurlUPtr curl;
    std::optional<CurlMConnector> curlMConnector;
    bool transferDone{};

    bool statusLineExpected{true};
    int statusCode{};
    std::optional<std::int64_t> contentLength;

    char* dst{};
    const char* dstEnd{};

    std::string buf;
    std::size_t bufPos{};

    [[noreturn]]
    void throwError(const char* description, CURLcode code);
    [[noreturn]]
    void throwError(const char* description, CURLMcode code);

    void headerFn(std::string_view data);

    static std::size_t curlHeaderFn(
        char* data,
        std::size_t itemSize,
        std::size_t numItems,
        void* userData)
    {
        const auto dataSize = itemSize * numItems;
        static_cast<CurlResponse*>(userData)->headerFn(
            {data, dataSize});
        return dataSize;
    }

    void writeFn(std::string_view data);

    static std::size_t curlWriteFn(
        char* data,
        std::size_t itemSize,
        std::size_t numItems,
        void* userData)
    {
        const auto dataSize = itemSize * numItems;
        static_cast<CurlResponse*>(userData)->writeFn(
            {data, dataSize});
        return dataSize;
    }

    void performTransferStep();
    void read();
};


CurlResponse::CurlResponse(
        std::string_view url, std::string_view userAgent)
    : libCurl{LibCurl::get()}
    , curlM{{}, CurlMDeleter{libCurl}}
    , curl{{}, CurlDeleter{libCurl}}
{
    curlM.reset(libCurl.multi_init());
    if (!curlM)
        throw Error{"curl_multi_init() failed"};

    curl.reset(libCurl.easy_init());
    if (!curl)
        throw Error{"curl_easy_init() failed"};

    // cURL allows you to get an option name string by its CURLOPT_ id
    // via curl_easy_option_by_id() added in 7.73.0 (Oct 2020).
    // However, we want to support older versions, so use macro
    // stringification instead.
    #define SETOPT(OPT, PARAM) \
    do { \
        const auto code = libCurl.easy_setopt(curl.get(), OPT, PARAM); \
        if (code != CURLE_OK) \
            throwError("curl_easy_setopt() " #OPT, code); \
    } while (false)

    SETOPT(CURLOPT_ERRORBUFFER, curlError);

    SETOPT(CURLOPT_URL, std::string{url}.c_str());
    SETOPT(CURLOPT_USERAGENT, std::string{userAgent}.c_str());

    SETOPT(CURLOPT_HEADERFUNCTION, curlHeaderFn);
    SETOPT(CURLOPT_HEADERDATA, this);

    SETOPT(CURLOPT_WRITEFUNCTION, curlWriteFn);
    SETOPT(CURLOPT_WRITEDATA, this);

    // It is crucial to use a timeout during the transfer, as cURL
    // doesn't use one by default. Otherwise, a curl_multi loop will
    // run indefinitely if, e.g., the connection is terminated.
    //
    // We can use either the CURLOPT_LOW_SPEED_LIMIT/TIME pair, or
    // measure the timeout ourselves. Note that:
    //
    // * CURLOPT_CONNECTTIMEOUT only limits the connection phase and
    //   has no effect during the transfer.
    //
    // * curl_multi_wait/poll() accepts a timeout, but doesn't let you
    //   to determine whether it returned due to the timeout or lack
    //   of activity. That is, it can return a zero numfds several
    //   times during the transfer even if there's an active easy
    //   handle.
    SETOPT(CURLOPT_LOW_SPEED_LIMIT, 1l);
    SETOPT(CURLOPT_LOW_SPEED_TIME, 10l);

    curlMConnector.emplace(libCurl, curlM.get(), curl.get());

    // Fetch the first chunk of the response body explicitly to make
    // sure we handled the header.
    while (!transferDone && buf.empty())
        performTransferStep();

    if (statusCode != 200)
        throw Error{str::format("HTTP status code {}", statusCode)};
}


[[noreturn]]
void CurlResponse::throwError(const char* description, CURLcode code)
{
    assert(code != CURLE_OK);

    const auto message = str::format(
        "{}: {}",
        description,
        *curlError ? curlError : libCurl.easy_strerror(code));

    // cURL versions before 7.60.0 don't clear the buffer before
    // returning an error code if there are no error details, so we do
    // it ourselves. Of course, this will force subsequent calls to
    // throwError() to degrade to curl_easy_strerror(), but it's still
    // better than an irrelevant error message.
    curlError[0] = 0;

    switch (code) {
    case CURLE_COULDNT_RESOLVE_HOST:
    case CURLE_OPERATION_TIMEDOUT:
        throw ConnectionError{message};
    default:
        throw Error{message};
    }
}


[[noreturn]]
void CurlResponse::throwError(const char* description, CURLMcode code)
{
    assert(code != CURLM_OK);
    throw Error{str::format(
        "{}: {}", description, libCurl.multi_strerror(code))};
}


void CurlResponse::headerFn(std::string_view data)
{
    if (!buf.empty())
        // This is a trailer of the chunked transfer coding.
        return;

    const std::string_view crlf{"\r\n"};

    if (data == crlf) {
        // cURL invokes the function for the headers of all responses
        // received after initiating a request, and not just the final
        // one. This is the end of the current response header, so
        // prepare for the next one, if any.
        statusLineExpected = true;
        return;
    }

    if (statusLineExpected) {
        statusLineExpected = false;

        // Drop all data extracted from the previous response, if any.
        statusCode = 0;
        contentLength = {};

        if (const auto spacePos = data.find(' ');
                spacePos != data.npos)
            std::from_chars(
                data.data() + spacePos + 1,
                data.data() + data.size(),
                statusCode);

        return;
    }

    const auto colonPos = data.find(':');
    if (colonPos == data.npos
        || !str::equalIgnoreCase(
            "Content-Length", data.substr(0, colonPos)))
        return;

    auto val = data.substr(colonPos + 1);

    // cURL doesn't exclude CRLF from headers.
    if (str::endsWith(val, crlf))
        val.remove_suffix(crlf.size());

    // According to RFC 9112, a header value may contain leading
    // or trailing whitespace (spaces and horizontal tabs), which
    // should be ignored.
    val = str::trim(val, str::isBlank);

    std::int64_t contentLength{};
    const auto [ptr, ec] = std::from_chars(
        val.data(), val.data() + val.size(), contentLength);
    if (ec == std::errc{}
            && ptr == val.data() + val.size()
            && contentLength >= 0)
        this->contentLength = contentLength;
}


void CurlResponse::writeFn(std::string_view data)
{
    // We are going to write directly to dst first, so the buf data
    // must not be partially drained. At the same time, we don't
    // expect the write function to be called exactly once per
    // curl_multi_perform(), so buf can already be nonempty.
    assert(bufPos == 0);

    // dst can be null when we call performTransferStep() from the
    // constructor rather than from read().
    if (!dst) {
        assert(!dstEnd);
        buf.append(data);
        return;
    }

    assert(dstEnd);
    assert(dst <= dstEnd);

    const auto numWrite = data.copy(dst, dstEnd - dst);
    dst += numWrite;
    buf.append(data, numWrite);
}


void CurlResponse::performTransferStep()
{
    if (transferDone)
        return;

    auto code = libCurl.multi_wait(
        curlM.get(), nullptr, 0, 500, nullptr);
    if (code != CURLM_OK)
        throwError("curl_multi_wait()", code);

    int numRunningHandles{};
    code = libCurl.multi_perform(curlM.get(), &numRunningHandles);
    if (code != CURLM_OK)
        throwError("curl_multi_perform()", code);

    int numMsgsInQueue{};
    while (const auto* msg = libCurl.multi_info_read(
            curlM.get(), &numMsgsInQueue)) {
        assert(msg->easy_handle == curl.get());

        if (msg->msg != CURLMSG_DONE)
            continue;

        transferDone = true;

        if (msg->data.result != CURLE_OK)
            throwError(
                "curl_multi_perform() easy handle done with error",
                msg->data.result);
    }
}


std::size_t CurlResponse::read(void* dst, std::size_t dstSize)
{
    if (!dst)
        return 0;

    auto* dstBegin = static_cast<char*>(dst);

    this->dst = dstBegin;
    dstEnd = dstBegin + dstSize;

    read();

    return this->dst - dstBegin;
}


void CurlResponse::read()
{
    assert(dst);
    assert(dstEnd);
    assert(dst <= dstEnd);

    assert(bufPos <= buf.size());
    if (bufPos < buf.size()) {
        const auto numRead = buf.copy(dst, dstEnd - dst, bufPos);

        bufPos += numRead;
        dst += numRead;

        if (bufPos == buf.size()) {
            buf.clear();
            bufPos = 0;
        }
    }

    while (!transferDone && dst < dstEnd)
        performTransferStep();
}


}


std::unique_ptr<Response> makeGetRequest(
    std::string_view url, std::string_view userAgent)
{
    if (const auto& errorText = curlGlobalInit.getErrorText();
            !errorText.empty())
        throw Error{"libcurl initialization failed: " + errorText};

    return std::make_unique<CurlResponse>(url, userAgent);
}


}
