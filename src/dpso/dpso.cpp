
#include "dpso.h"

#include <cstddef>
#include <stdexcept>

#include "backend/backend.h"
#include "backend/backend_error.h"


static std::unique_ptr<dpso::backend::Backend> backend;


namespace dpso {

namespace hotkeys {
void init(dpso::backend::Backend& backend);
void shutdown();
}

namespace ocr {
void init(dpso::backend::Backend& backend);
void shutdown();
}

namespace selection {
void init(dpso::backend::Backend& backend);
void shutdown();
}

}


namespace {


struct Module {
    const char* name;
    void (*init)(dpso::backend::Backend&);
    void (*shutdown)();
};


}


#define MODULE(name) {#name, dpso::name::init, dpso::name::shutdown}

const Module modules[] = {
    MODULE(hotkeys),
    MODULE(ocr),
    MODULE(selection),
};

#undef MODULE


const auto numModules = sizeof(modules) / sizeof(*modules);


int dpsoInit(void)
{
    try {
        backend = dpso::backend::Backend::create();
    } catch (dpso::backend::BackendError& e) {
        dpsoSetError("Can't create backend: %s", e.what());
        return false;
    }

    for (std::size_t i = 0; i < numModules; ++i)
        try {
            modules[i].init(*backend);
        } catch (std::runtime_error& e) {
            dpsoSetError(
                "Can't init %s: %s", modules[i].name, e.what());

            for (auto j = i; j--;)
                modules[j].shutdown();

            backend.reset();
            return false;
        }

    return true;
}


void dpsoShutdown(void)
{
    for (auto i = numModules; i--;)
        modules[i].shutdown();

    backend.reset();
}


void dpsoUpdate(void)
{
    if (backend)
        backend->update();
}
