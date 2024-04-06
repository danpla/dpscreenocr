
#include "request.h"

#include <algorithm>
#include <cassert>
#include <charconv>
#include <string>

#include <curl/curl.h>

#include "dpso_utils/str.h"

#include "error.h"
#include "request_curl_lib.h"


namespace dpso::net {
namespace {


// Note that we never call LibCurl::get() from destructors and
// unique_ptr deleters to avoid confusing static analysis tools. They
// don't know that such calls will not throw, since there is always a
// previous LibCurl::get() to create a cURL object or access
// curl_global_init().


// curl_global_init() docs say:
//
//   [...] you must not call this function when any other thread in
//   the program (i.e. a thread sharing the same memory) is running.
//   This does not just mean no other thread that is using libcurl.
//   Because curl_global_init calls functions of other libraries that
//   are similarly thread unsafe, it could conflict with any other
//   thread that uses these other libraries.
//
// The documentation suggest to call curl_global_init/cleanup() either
// from the very begin/end of main(), or from constructor/destructor
// of a global static C++ object.
//
// For details, see:
// * https://curl.se/libcurl/c/curl_global_init.html
// * "Global constants" at https://curl.se/libcurl/c/libcurl.html
class CurlGlobalInit {
public:
    CurlGlobalInit()
    {
        try {
            libCurl = &LibCurl::get();
        } catch (Error& e) {
            errorText = e.what();
            return;
        }

        const auto code = libCurl->global_init(CURL_GLOBAL_ALL);
        if (code != CURLE_OK)
            errorText = str::format(
                "curl_global_init: {}", libCurl->easy_strerror(code));
    }

    ~CurlGlobalInit()
    {
        if (libCurl && errorText.empty())
            libCurl->global_cleanup();
    }

    // The text is empty if there was no error.
    const std::string& getErrorText() const
    {
        return errorText;
    }
private:
    const LibCurl* libCurl;
    std::string errorText;
};


const CurlGlobalInit curlGlobalInit;


class CurlMDeleter {
public:
    explicit CurlMDeleter(const LibCurl& libCurl)
        : libCurl{libCurl}
    {
    }

    void operator()(CURLM* curlM) const
    {
        libCurl.multi_cleanup(curlM);
    }
private:
    const LibCurl& libCurl;
};


using CurlMUPtr = std::unique_ptr<CURLM, CurlMDeleter>;


class CurlDeleter {
public:
    explicit CurlDeleter(const LibCurl& libCurl)
        : libCurl{libCurl}
    {
    }

    void operator()(CURL* curl) const
    {
        libCurl.easy_cleanup(curl);
    }
private:
    const LibCurl& libCurl;
};


using CurlUPtr = std::unique_ptr<CURL, CurlDeleter>;


class CurlMConnector {
public:
    CurlMConnector(const LibCurl& libCurl, CURLM* curlM, CURL* curl)
        : libCurl{libCurl}
        , curlM{curlM}
        , curl{curl}
    {
        libCurl.multi_add_handle(curlM, curl);
    }

    ~CurlMConnector()
    {
        libCurl.multi_remove_handle(curlM, curl);
    }
private:
    const LibCurl& libCurl;
    CURLM* curlM;
    CURL* curl;
};


class CurlResponse : public Response {
public:
    CurlResponse(const char* url, const char* userAgent);

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
    std::unique_ptr<CurlMConnector> curlMConnector;
    bool transferDone{};

    bool statusLineExpected{true};
    int statusCode{};
    std::optional<std::int64_t> contentLength;

    char* dst{};
    std::size_t dstSize{};
    std::size_t dstPos{};

    std::string buf;
    std::size_t bufPos{};

    [[noreturn]]
    void throwError(const char* description, CURLcode code);
    [[noreturn]]
    void throwError(const char* description, CURLMcode code);

    static std::size_t curlHeaderFn(
        char* data,
        std::size_t itemSize,
        std::size_t numItems,
        void* userData);

    static std::size_t curlWriteFn(
        char* data,
        std::size_t itemSize,
        std::size_t numItems,
        void* userData);

    void performTransferStep();

    std::size_t read();
};


CurlResponse::CurlResponse(const char* url, const char* userAgent)
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

    #ifndef NDEBUG
    SETOPT(CURLOPT_VERBOSE, 1l);
    #endif

    SETOPT(CURLOPT_ERRORBUFFER, curlError);

    SETOPT(CURLOPT_URL, url);
    SETOPT(CURLOPT_USERAGENT, userAgent);

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

    curlMConnector = std::make_unique<CurlMConnector>(
        libCurl, curlM.get(), curl.get());

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


std::size_t CurlResponse::curlHeaderFn(
    char* data,
    std::size_t itemSize,
    std::size_t numItems,
    void* userData)
{
    assert(userData);
    auto& resp = *static_cast<CurlResponse*>(userData);

    const auto dataSize = itemSize * numItems;

    if (!resp.buf.empty())
        // This is a trailer of the chunked transfer coding.
        return dataSize;

    const auto dataEnd = data + dataSize;

    if (dataSize == 2 && data[0] == '\r' && data[1] == '\n') {
        // cURL invokes the function for the headers of all responses
        // received after initiating a request, and not just the final
        // one. This is the end of the current response header, so
        // prepare for the next one, if any.
        resp.statusLineExpected = true;
    } else if (resp.statusLineExpected) {
        resp.statusLineExpected = false;

        // Drop all data extracted from the previous response, if any.
        resp.statusCode = 0;
        resp.contentLength = {};

        if (const auto spacePos = std::find(data, dataEnd, ' ');
                spacePos != dataEnd)
            std::from_chars(
                spacePos + 1, dataEnd, resp.statusCode, 10);
    } else if (const auto colonPos = std::find(data, dataEnd, ':');
            colonPos != dataEnd
            && str::cmpSubStr(
                "Content-Length",
                data,
                colonPos - data,
                str::cmpIgnoreCase) == 0) {
        // According to RFC 9112, a header value may contain leading
        // or trailing whitespace (spaces and horizontal tabs), which
        // should be ignored.
        const auto* valBegin = colonPos + 1;
        while (valBegin < dataEnd && str::isBlank(*valBegin))
            ++valBegin;

        const auto* valEnd = dataEnd;

        // cURL doesn't exclude CRLF from headers.
        if (valEnd - valBegin > 1
                && valEnd[-2] == '\r'
                && valEnd[-1] == '\n')
            valEnd -= 2;

        while (valBegin < valEnd && str::isBlank(valEnd[-1]))
            --valEnd;

        std::int64_t contentLength{};
        const auto [ptr, ec] = std::from_chars(
            valBegin, valEnd, contentLength, 10);
        if (ec == std::errc{} && ptr == valEnd && contentLength >= 0)
            resp.contentLength = contentLength;
    }

    return dataSize;
}


std::size_t CurlResponse::curlWriteFn(
    char* data,
    std::size_t itemSize,
    std::size_t numItems,
    void* userData)
{
    assert(userData);
    auto& resp = *static_cast<CurlResponse*>(userData);

    const auto dataSize = itemSize * numItems;

    // We are going to write directly to dst first, so the buf data
    // must not be partially drained. At the same time, we don't
    // expect the write function to be called exactly once per
    // curl_multi_perform(), so buf may already be nonempty.
    assert(resp.bufPos == 0);

    // We allow dst to be null of dstSize is zero so that we can use
    // performTransferStep() from the constructor.
    assert(resp.dstSize == 0 || resp.dst);

    assert(resp.dstPos <= resp.dstSize);
    const auto numWrite = std::min(
        dataSize, resp.dstSize - resp.dstPos);

    std::copy_n(data, numWrite, resp.dst + resp.dstPos);
    resp.dstPos += numWrite;

    resp.buf.append(data + numWrite, dataSize - numWrite);

    return dataSize;
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
    this->dst = static_cast<char*>(dst);
    this->dstSize = dstSize;
    dstPos = 0;

    return read();
}


std::size_t CurlResponse::read()
{
    assert(dst);
    assert(dstPos == 0);

    assert(bufPos <= buf.size());
    if (bufPos < buf.size()) {
        const auto numRead = std::min(dstSize, buf.size() - bufPos);
        buf.copy(dst, numRead, bufPos);

        bufPos += numRead;
        dstPos = numRead;

        if (bufPos == buf.size()) {
            buf.clear();
            bufPos = 0;
        }
    }

    while (!transferDone && dstPos < dstSize)
        performTransferStep();

    return dstPos;
}


}


std::unique_ptr<Response> makeGetRequest(
    const char* url, const char* userAgent)
{
    if (const auto& errorText = curlGlobalInit.getErrorText();
            !errorText.empty())
        throw Error{"libcurl initialization failed: " + errorText};

    return std::make_unique<CurlResponse>(url, userAgent);
}


}
