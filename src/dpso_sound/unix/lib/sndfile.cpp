#include "unix/lib/sndfile.h"

#include <dlfcn.h>

#include "dpso_utils/str.h"
#include "dpso_utils/unix/dl.h"

#include "error.h"


namespace dpso::sound {
namespace {


unix::DlHandleUPtr loadLib(const char* soName)
{
    if (auto* handle = dlopen(soName, RTLD_NOW | RTLD_LOCAL))
        return unix::DlHandleUPtr{handle};

    throw Error{str::format("Can't load {}: {}", soName, dlerror())};
}


void* loadFn(const char* name)
{
    static const auto* libName = "libsndfile.so.1";
    static const auto dlHandle = loadLib(libName);

    if (auto* result = dlsym(dlHandle.get(), name))
        return result;

    throw Error{str::format(
        "dlsym for \"{}\" from \"{}\": {}",
        name, libName, dlerror())};
}


}


const LibSndfile& LibSndfile::get()
{
    static const LibSndfile lib;
    return lib;
}


#define LOAD_FN(NAME) NAME{(decltype(NAME))loadFn("sf_" #NAME)}


LibSndfile::LibSndfile()
    : LOAD_FN(open)
    , LOAD_FN(command)
    , LOAD_FN(readf_short)
    , LOAD_FN(close)
    , LOAD_FN(strerror)
{
}


}
