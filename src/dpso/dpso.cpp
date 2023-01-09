
#include "dpso.h"

#include <cstddef>
#include <iterator>
#include <stdexcept>

#include "backend/backend.h"
#include "backend/backend_error.h"


static std::unique_ptr<dpso::backend::Backend> backend;


#define DECL_MODULE_FUNCTIONS(NAME) \
namespace NAME { \
void init(dpso::backend::Backend& backend); \
void shutdown(); \
}

namespace dpso {
DECL_MODULE_FUNCTIONS(keyManager)
DECL_MODULE_FUNCTIONS(ocr)
DECL_MODULE_FUNCTIONS(selection)
}

#undef DECL_MODULE_FUNCTIONS


namespace {


struct Module {
    const char* name;
    void (*init)(dpso::backend::Backend&);
    void (*shutdown)();
};


}


#define MODULE(NAME) {#NAME, dpso::NAME::init, dpso::NAME::shutdown}

const Module modules[] = {
    MODULE(keyManager),
    MODULE(ocr),
    MODULE(selection),
};

#undef MODULE


const auto numModules = std::size(modules);


bool dpsoInit(void)
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
