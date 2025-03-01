#pragma once

#include <curl/curl.h>


namespace dpso::net {


class LibCurl {
    LibCurl();
public:
    // Throws net::Error if the curl library or one of its functions
    // cannot be loaded.
    static const LibCurl& get();

    #define DECL_FN(NAME) const decltype(&::curl_ ## NAME) NAME

    DECL_FN(easy_cleanup);
    DECL_FN(easy_init);
    DECL_FN(easy_setopt);
    DECL_FN(easy_strerror);
    DECL_FN(global_cleanup);
    DECL_FN(global_init);
    DECL_FN(multi_add_handle);
    DECL_FN(multi_cleanup);
    DECL_FN(multi_info_read);
    DECL_FN(multi_init);
    DECL_FN(multi_perform);
    DECL_FN(multi_remove_handle);
    DECL_FN(multi_strerror);
    DECL_FN(multi_wait);

    #undef DECL_FN
};


}
