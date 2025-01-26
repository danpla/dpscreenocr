#include "unix/lib_pulse.h"

#include <dlfcn.h>

#include "dpso_utils/str.h"
#include "dpso_utils/unix/dl.h"

#include "sound.h"


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


void* loadLibPulseFn(const char* name)
{
    static const auto libInfo = loadLib("libpulse.so.0");
    return loadFn(libInfo, name);
}


void* loadLibPulseSimpleFn(const char* name)
{
    static const auto libInfo = loadLib("libpulse-simple.so.0");
    return loadFn(libInfo, name);
}


}


const LibPulse& LibPulse::get()
{
    static const LibPulse lib;
    return lib;
}


#define LOAD_FN(NAME) \
    NAME{(decltype(NAME))loadLibPulseFn("pa_" #NAME)}

#define LOAD_SIMPLE_FN(NAME) \
    NAME{(decltype(NAME))loadLibPulseSimpleFn("pa_" #NAME)}


LibPulse::LibPulse()
    : LOAD_FN(strerror)
    , LOAD_FN(get_binary_name)
    , LOAD_SIMPLE_FN(simple_new)
    , LOAD_SIMPLE_FN(simple_free)
    , LOAD_SIMPLE_FN(simple_write)
    , LOAD_SIMPLE_FN(simple_drain)
    , LOAD_SIMPLE_FN(simple_flush)
{
}


}
