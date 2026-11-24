#include "request.h"

#include <algorithm>
#include <cassert>
#include <charconv>
#include <cstdint>
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
    const auto lastError = GetLastError();

    const auto message = str::format(
        "{}: {} ({})",
        info,
        windows::getErrorMessage(
            lastError, GetModuleHandleW(L"wininet")),
        // Include the code so that we can do a quick search on
        // https://learn.microsoft.com/en-us/windows/win32/wininet/wininet-errors
        lastError);

    switch (lastError) {
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


// Using the HTTP_QUERY_FLAG_NUMBER flag (which implies DWORD) for
// the "Content-Length" header field limits the size to 4 GB. The
// 64-bit version of this flag (HTTP_QUERY_FLAG_NUMBER64) solves the
// problem, but it was only added in one of the Windows 10 versions
// and is not available on older systems. As a workaround, we query
// numeric fields as strings and parse numbers manually.


std::optional<std::string> queryStrInfo(
    HINTERNET hConnection, DWORD infoLevel)
{
    std::wstring str;

    // HttpQueryInfoW() null-terminates the string. The data pointer
    // of an empty std::wstring points to the terminating null, so we
    // include it in dataSize to avoid a second HttpQueryInfoW() call
    // for empty strings, i.e., when the routine only needs to write
    // the terminating null.
    auto* data = str.data();
    DWORD dataSize{sizeof(wchar_t)};

    const auto queryInfo = [&]()
    {
        return HttpQueryInfoW(
            hConnection, infoLevel, data, &dataSize, nullptr);
    };

    if (queryInfo())
        return std::string{};

    if (const auto lastError = GetLastError();
            lastError == ERROR_HTTP_HEADER_NOT_FOUND)
        return {};
    else if (lastError != ERROR_INSUFFICIENT_BUFFER)
        throwLastError("HttpQueryInfoW");

    // In case of ERROR_INSUFFICIENT_BUFFER (but not on success), the
    // returned size includes the terminating null.
    str.resize(dataSize / sizeof(wchar_t) - 1);
    data = str.data();

    if (!queryInfo())
        throwLastError("HttpQueryInfoW");

    try {
        return windows::utf16ToUtf8(str);
    } catch (windows::CharConversionError& e) {
        throw Error{str::format(
            "Can't convert header field from UTF-16 to UTF-8: {}",
            e.what())};
    }
}


std::optional<std::int64_t> queryNumInfo(
    HINTERNET hConnection, DWORD infoLevel)
{
    const auto str = queryStrInfo(hConnection, infoLevel);
    if (!str)
        return {};

    const auto* begin = str->data();
    const auto* end = begin + str->size();
    std::int64_t n;

    const auto [ptr, ec] = std::from_chars(begin, end, n);
    if (ec == std::errc{} && ptr == end)
        return n;

    if (ec == std::errc::result_out_of_range)
        throw Error{str::format(
            "Number {}[...] is out of range",
            std::string{begin, ptr})};

    throw Error{str::format("Invalid number format \"{}\"", *str)};
}


int getStatusCode(HINTERNET hConnection)
{
    try {
        if (const auto result = queryNumInfo(
                hConnection, HTTP_QUERY_STATUS_CODE))
            return *result;
    } catch (Error& e) {
        throw Error{str::format(
            "queryNumInfo(HTTP_QUERY_STATUS_CODE): {}", e.what())};
    }

    throw Error{"No status code (HTTP_QUERY_STATUS_CODE)"};
}


std::optional<std::int64_t> getContentLength(HINTERNET hConnection)
{
    std::optional<std::int64_t> result;

    try {
        result = queryNumInfo(hConnection, HTTP_QUERY_CONTENT_LENGTH);
    } catch (Error& e) {
        throw Error{str::format(
            "queryNumInfo(HTTP_QUERY_CONTENT_LENGTH): {}", e.what())};
    }

    if (result && *result < 0)
        throw Error{str::format(
            "Negative Content-Length ({})", *result)};

    return result;
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
    std::vector<std::uint8_t> buf;
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
                static_cast<std::uint8_t*>(dst) + dstPos);

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
                static_cast<std::uint8_t*>(dst) + dstPos,
                availDstSize,
                &numRead))
            return dstPos + numRead;

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            throwLastError("InternetReadFile (direct)");

        // Note that we don't increase the capacity if it's already
        // big enough: since it was fine for the previous run, there's
        // a chance that it will work for the current one.
        buf.reserve(
            std::max<std::size_t>(16 * 1024, availDstSize * 2));
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
    std::string_view url, std::string_view userAgent)
{
    std::wstring userAgentUtf16;
    try {
        userAgentUtf16 = windows::utf8ToUtf16(userAgent);
    } catch (windows::CharConversionError& e) {
        throw Error{str::format(
            "Can't convert userAgent to UTF-16: {}", e.what())};
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
    } catch (windows::CharConversionError& e) {
        throw Error{str::format(
            "Can't convert URL to UTF-16: {}", e.what())};
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
        throw Error{str::format("HTTP status code {}", statusCode)};

    return std::make_unique<WindowsResponse>(
        std::move(hInternet), std::move(hConnection));
}


}
