#pragma once

#include <memory>
#include <string>

#include "error.h"
#include "request_curl_lib.h"


namespace dpso::net {


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
            errorText =
                std::string{"curl_global_init: "}
                + libCurl->easy_strerror(code);
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
    const LibCurl* libCurl{};
    std::string errorText;
};


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


}
