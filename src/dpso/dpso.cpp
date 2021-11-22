
#include "dpso.h"

#include <cstddef>
#include <stdexcept>
#include <string>

#include "backend/backend.h"
#include "ocr_private.h"


namespace {


struct Module {
    const char* name;
    void (*init)();
    void (*shutdown)();
};


}


#define MODULE(name) {#name, dpso::name::init, dpso::name::shutdown}

const Module modules[] = {
    MODULE(backend),
    MODULE(ocr),
};

#undef MODULE


const auto numModules = sizeof(modules) / sizeof(*modules);


int dpsoInit(void)
{
    for (std::size_t i = 0; i < numModules; ++i)
        try {
            modules[i].init();
        } catch (std::runtime_error& e) {
            dpsoSetError((
                std::string{"Can't init "} + modules[i].name + ": "
                + e.what()).c_str());

            for (auto j = i; j--;)
                modules[j].shutdown();

            return false;
        }

    return true;
}


void dpsoShutdown(void)
{
    for (auto i = numModules; i--;)
        modules[i].shutdown();
}


void dpsoUpdate(void)
{
    dpso::backend::getBackend().update();
}
