
#include "request_curl_lib.h"

#if DPSO_DYNAMIC_CURL
#include <dlfcn.h>
#endif

#include <memory>

#include "dpso_utils/str.h"

#include "error.h"


namespace dpso::net {
namespace {


#if DPSO_DYNAMIC_CURL

// When the application is shipped in a self-contained package for
// Unix-like systems, it's necessary not to bundle libcurl, but
// instead to load the system version dynamically.
//
// The main reason is TLS certificates. They all have expiration
// dates, and if we bundle them in the package, the networking part
// will break one day. One solution would be to tell the bundled
// libcurl a path to the location of system certificates, but this
// path is not standardized across different GNU/Linux distributions.
// Another obvious choice is to just let the application to link
// against the system libcurl, but this will not work for at least two
// reasons:
//
// * Modern systems ship with different flavors of libcurl, depending
//   on the TLS backend used. For example, Debian and derivatives have
//   OpenSSL (libcurl.so.*), GnuTLS (libcurl-gnutls.so.*), and NSS
//   (libcurl-nss.so.*) flavors. Moreover, the GnuTLS flavor is
//   actually the default version on Debian, as it's installed in the
//   stock system as a dependency of standard programs like network-
//   manager.
//
// * Even if we could assume that the user has the OpenSSL flavor of
//   libcurl (libcurl.so.*), the linkage can still fail due to
//   versioned symbols - not only on other distributions, but also on
//   newer versions of the same system. For example, a program built
//   on Ubuntu 14.04 will fail to link on 20.04 because the version
//   "CURL_OPENSSL_3" will not be found.


struct DlHandleCloser {
    void operator()(void* handle) const
    {
        dlclose(handle);
    }
};


using DlHandleUPtr = std::unique_ptr<void, DlHandleCloser>;


struct LibInfo {
    std::string name;
    DlHandleUPtr handle;
};


LibInfo loadLib()
{
    // As of this writing, the current SO version is 4, bumped in
    // libcurl 7.16.0. The set of functions we will load requires a
    // much newer libcurl, so we don't try to load libraries with
    // older SO versions.
    //
    // https://curl.se/libcurl/abi.html
    const auto minSoVersion = 4;

    // To make the code more future-proof, we also try to load
    // libraries with newer SO versions, hoping that the ABI changes
    // introduced in them won't affect the functions we load.
    const auto maxSoVersion = minSoVersion + 5;

    const char* names[] = {
        "libcurl",
        "libcurl-gnutls",
        "libcurl-nss",
    };

    for (auto v = minSoVersion; v <= maxSoVersion; ++v)
        for (const auto* name : names) {
            const auto soName = str::format("{}.so.{}", name, v);
            if (auto* handle = dlopen(
                    soName.c_str(), RTLD_NOW | RTLD_LOCAL))
                return {soName, DlHandleUPtr{handle}};
        }

    std::string triedSoNames;
    for (const auto* name : names) {
        if (!triedSoNames.empty())
            triedSoNames += ", ";

        triedSoNames += str::format(
            "{}.so.[{}-{}]", name, minSoVersion, maxSoVersion);
    }

    throw Error{str::format(
        "Can't load libcurl. Tried names: {}. Last error: {}",
        triedSoNames, dlerror())};
}


void* loadFn(const char* name)
{
    static auto libInfo = loadLib();

    if (auto* result = dlsym(libInfo.handle.get(), name))
        return result;

    throw Error{str::format(
        "dlsym for \"{}\" from \"{}\": {}",
        name, libInfo.name, dlerror())};
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
