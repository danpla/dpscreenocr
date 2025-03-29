#include "unix/lib/sndfile.h"

#include <dlfcn.h>

#include "dpso_utils/str.h"
#include "dpso_utils/unix/dl.h"

#include "error.h"


namespace dpso::sound {
namespace {


struct LibInfo {
    std::string name;
    unix::DlHandleUPtr handle;
};


LibInfo loadLib(const char* soName)
{
    if (auto* handle = dlopen(soName, RTLD_NOW | RTLD_LOCAL))
        return {soName, unix::DlHandleUPtr{handle}};

    throw Error{str::format("Can't load {}: {}", soName, dlerror())};
}


void* loadFn(const LibInfo& libInfo, const char* name)
{
    if (auto* result = dlsym(libInfo.handle.get(), name))
        return result;

    throw Error{str::format(
        "dlsym for \"{}\" from \"{}\": {}",
        name, libInfo.name, dlerror())};
}


void* loadLibSndfileFn(const char* name)
{
    static const auto libInfo = loadLib("libsndfile.so.1");
    return loadFn(libInfo, name);
}


}


const LibSndfile& LibSndfile::get()
{
    static const LibSndfile lib;
    return lib;
}


#define LOAD_FN(NAME) \
    NAME{(decltype(NAME))loadLibSndfileFn("sf_" #NAME)}


LibSndfile::LibSndfile()
    : LOAD_FN(open)
    , LOAD_FN(command)
    , LOAD_FN(readf_short)
    , LOAD_FN(close)
    , LOAD_FN(strerror)
{
}


}
