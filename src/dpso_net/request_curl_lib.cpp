
#include "request_curl_lib.h"

#ifdef DPSO_DYNAMIC_CURL
#include <dlfcn.h>
#endif

#include <memory>

#include "dpso_utils/str.h"

#include "error.h"


namespace dpso::net {
namespace {


#if DPSO_DYNAMIC_CURL

// When the application is shipped in a self-contained package for
// Unix-like systems (like AppImage), it's necessary not to bundle
// libcurl, but instead to load the system version dynamically.
//
// The main reason are TLS certificates. They all have expiration
// dates, and if we bundle them in the package, the networking part
// will one day become broken.
//
// One solution would be to tell the bundled libcurl a path to the
// system certificates location, but this path is not standardized
// across various GNU/Linux distributions.
//
// Another obvious choice is to just let the application to link with
// system libcurl, but this will not work for at least two reasons:
//
// 1. Modern systems are shipped with various flavors of libcurl,
//    depending on the used TLS backend. For example, Debian and
//    derivatives have OpenSSL (libcurl.so.*), GnuTLS
//    (libcurl-gnutls.so), and NSS (libcurl-nss.so.*) flavors.
//    Moreover, the GnuTLS flavor is in fact the default version on
//    Debian, as it's installed in the stock system as a dependency of
//    default programs like network-manager.
//
// 2. Even if we could assume that the user has a OpenSSL flavor
//    of libcurl (libcurl.so.*), the linkage can still fail due to
//    versioned symbols in the library - not only on other
//    distributions, but also on a new version of the same system. For
//    example, a program built on Ubuntu 14.04 will fail to link on
//    Ubuntu 20.04 because the version "CURL_OPENSSL_3" will not be
//    found.


struct DlHandleCloser {
    void operator()(void* handle) const
    {
        dlclose(handle);
    }
};


using DlHandleUPtr = std::unique_ptr<void, DlHandleCloser>;


DlHandleUPtr loadLib()
{
    const char* names[] = {
        "libcurl.so",
        "libcurl.so.3",
        "libcurl.so.4",
        "libcurl-gnutls.so.3",
        "libcurl-gnutls.so.4",
        "libcurl-nss.so.3",
        "libcurl-nss.so.4",
    };

    for (const auto* name : names)
        if (auto* handle = dlopen(name, RTLD_NOW))
            return DlHandleUPtr{handle};

    std::string namesStr;
    for (const auto* name : names) {
        if (!namesStr.empty())
            namesStr += ", ";

        namesStr += name;
    }

    throw Error{str::printf(
        "Can't load libcurl. Tried names: %s. Last error: %s",
        namesStr.c_str(),
        dlerror())};
}


void* loadFn(const char* name)
{
    static auto dlHandle = loadLib();

    if (auto* result = dlsym(dlHandle.get(), name))
        return result;

    throw Error{str::printf("dlsym for \"%s\": %s", name, dlerror())};
}


#define LOAD_FN(NAME) \
    NAME{(decltype(&::curl_ ## NAME))loadFn("curl_" #NAME)}

#else

#define LOAD_FN(NAME) NAME{::curl_ ## NAME}

#endif


}


const LibCurl& LibCurl::get()
{
    static const LibCurl libCurl;
    return libCurl;
}


LibCurl::LibCurl()
    : LOAD_FN(easy_cleanup)
    , LOAD_FN(easy_init)
    , LOAD_FN(easy_setopt)
    , LOAD_FN(easy_strerror)
    , LOAD_FN(global_cleanup)
    , LOAD_FN(global_init)
    , LOAD_FN(multi_add_handle)
    , LOAD_FN(multi_cleanup)
    , LOAD_FN(multi_info_read)
    , LOAD_FN(multi_init)
    , LOAD_FN(multi_perform)
    , LOAD_FN(multi_remove_handle)
    , LOAD_FN(multi_strerror)
    , LOAD_FN(multi_wait)
{
}


}
