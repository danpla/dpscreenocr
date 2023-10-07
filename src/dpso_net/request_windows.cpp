
#include "request.h"

#include <algorithm>
#include <cassert>
#include <string>
#include <utility>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wininet.h>

#include "dpso_utils/str.h"
#include "dpso_utils/windows/error.h"
#include "dpso_utils/windows/utf.h"

#include "error.h"


namespace dpso::net {
namespace {


struct InternetCloser {
    using pointer = HINTERNET;

    void operator()(HINTERNET h) const
    {
        InternetCloseHandle(h);
    }
};


using InternetUPtr = std::unique_ptr<HINTERNET, InternetCloser>;


[[noreturn]]
void throwLastError(const char* info)
{
    const auto message = str::printf(
        "%s: %s (%lu)",
        info,
        windows::getErrorMessage(
            GetLastError(), GetModuleHandleW(L"wininet")).c_str(),
        // Include the code so that we can do a quick search on
        // https://learn.microsoft.com/en-us/windows/win32/wininet/wininet-errors
        GetLastError());

    switch (GetLastError()) {
    // Despite its name, ERROR_INTERNET_CANNOT_CONNECT is not reported
    // when there's no connection (ERROR_INTERNET_NAME_NOT_RESOLVED is
    // set in this case instead). I do not know under what
    // circumstances it is used, but the Internet says that one of the
    // reasons could be a wrong TLS configuration.
    case ERROR_INTERNET_CANNOT_CONNECT:  // 12029
        [[fallthrough]];
    // ERROR_INTERNET_NAME_NOT_RESOLVED is set by InternetOpenUrl()
    // when there's no connection.
    case ERROR_INTERNET_NAME_NOT_RESOLVED:  // 12007
        [[fallthrough]];
    // ERROR_INTERNET_TIMEOUT is set when the connection is
    // interrupted when using InternetReadFile().
    case ERROR_INTERNET_TIMEOUT:  // 12002
        throw ConnectionError{message};
    default:
        throw Error{message};
    }
}


DWORD getStatusCode(HINTERNET hConnection)
{
    DWORD result{};
    DWORD resultSize = sizeof(result);
    if (HttpQueryInfoW(
            hConnection,
            HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
            &result,
            &resultSize,
            0))
        return result;

    throwLastError("HttpQueryInfoW (HTTP_QUERY_STATUS_CODE)");
}


std::optional<std::int64_t> getContentLength(HINTERNET hConnection)
{
    DWORD result{};
    DWORD resultSize = sizeof(result);
    if (HttpQueryInfoW(
            hConnection,
            HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,
            &result,
            &resultSize,
            0))
        return result;

    if (GetLastError() == ERROR_HTTP_HEADER_NOT_FOUND)
        return {};

    throwLastError("HttpQueryInfoW (HTTP_QUERY_CONTENT_LENGTH)");
}


class WindowsResponse : public Response {
public:
    WindowsResponse(InternetUPtr hInternet, InternetUPtr hConnection)
        : hInternet{std::move(hInternet)}
        , hConnection{std::move(hConnection)}
        , contentLength{getContentLength(this->hConnection.get())}
    {
    }

    std::optional<std::int64_t> getSize() const override
    {
        return contentLength;
    }

    std::size_t read(void* dst, std::size_t dstSize) override;
private:
    InternetUPtr hInternet;
    InternetUPtr hConnection;
    std::optional<std::int64_t> contentLength;
    std::vector<unsigned char> buf;
    std::size_t bufPos{};
};


}


std::size_t WindowsResponse::read(void* dst, std::size_t dstSize)
{
    // When the response contains textual data, InternetReadFile()
    // reads one line at a time, returning ERROR_INSUFFICIENT_BUFFER
    // if the provided buffer is not large enough. We thus have to
    // make our own caching layer for such cases.

    std::size_t dstPos{};
    while (true) {
        assert(bufPos <= buf.size());

        if (bufPos < buf.size()) {
            assert(dstPos <= dstSize);

            const auto numRead = std::min(
                dstSize - dstPos, buf.size() - bufPos);
            std::copy_n(
                buf.data() + bufPos,
                numRead,
                static_cast<unsigned char*>(dst) + dstPos);

            bufPos += numRead;
            dstPos += numRead;
            if (dstPos == dstSize)
                return dstPos;
        }

        assert(dstPos <= dstSize);
        const auto availDstSize = dstSize - dstPos;

        // First try to read directly to dst to avoid unnecessary
        // copying.
        if (DWORD numRead; InternetReadFile(
                hConnection.get(),
                static_cast<unsigned char*>(dst) + dstPos,
                availDstSize,
                &numRead))
            return dstPos + numRead;

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            throwLastError("InternetReadFile (direct)");

        if (buf.empty())
            buf.reserve(
                std::max<std::size_t>(16 * 1024, availDstSize));
        // If the buffer is not empty, don't expand the capacity:
        // since it was fine for the previous run, there is a good
        // chance that it will also be suitable for the current one.

        buf.resize(buf.capacity());

        while (true) {
            if (DWORD numRead; InternetReadFile(
                    hConnection.get(),
                    buf.data(),
                    buf.size(),
                    &numRead)) {
                buf.resize(numRead);
                bufPos = 0;
                break;
            }

            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                throwLastError("InternetReadFile (buffered)");

            const auto newBufSize = buf.size() * 2;

            static const std::size_t bufSizeLimit{4 * 1024 * 1024};
            if (newBufSize > bufSizeLimit)
                throw Error{
                    "Suspiciously long text line in response"};

            buf.reserve(newBufSize);
            buf.resize(buf.capacity());
        }
    }
}


std::unique_ptr<Response> makeGetRequest(
    const char* url, const char* userAgent)
{
    std::wstring userAgentUtf16;
    try {
        userAgentUtf16 = windows::utf8ToUtf16(userAgent);
    } catch (std::runtime_error& e) {
        throw Error{str::printf(
            "Can't convert userAgent to UTF-16: %s", e.what())};
    }

    InternetUPtr hInternet{InternetOpenW(
        userAgentUtf16.c_str(),
        INTERNET_OPEN_TYPE_PRECONFIG,
        nullptr,
        nullptr,
        0)};
    if (!hInternet)
        throwLastError("InternetOpenW");

    std::wstring urlUtf16;
    try {
        urlUtf16 = windows::utf8ToUtf16(url);
    } catch (std::runtime_error& e) {
        throw Error{str::printf(
            "Can't convert URL to UTF-16: %s", e.what())};
    }

    InternetUPtr hConnection{InternetOpenUrlW(
        hInternet.get(),
        urlUtf16.c_str(),
        nullptr,
        0,
        INTERNET_FLAG_HYPERLINK
            | INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS
            | INTERNET_FLAG_KEEP_CONNECTION
            | INTERNET_FLAG_NO_CACHE_WRITE
            | INTERNET_FLAG_NO_COOKIES
            | INTERNET_FLAG_NO_UI
            | INTERNET_FLAG_RELOAD
            | INTERNET_FLAG_SECURE,
        0)};
    if (!hConnection)
        throwLastError("InternetOpenUrlW");

    const auto statusCode = getStatusCode(hConnection.get());
    if (statusCode != 200)
        throw Error{str::printf("HTTP status code %lu", statusCode)};

    return std::make_unique<WindowsResponse>(
        std::move(hInternet),
        std::move(hConnection));
}


}
