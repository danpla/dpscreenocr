
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


static std::string lastError;


int dpsoInit(void)
{
    lastError.clear();

    for (std::size_t i = 0; i < numModules; ++i)
        try {
            modules[i].init();
        } catch (std::runtime_error& e) {
            lastError = (
                std::string("Can't init ") + modules[i].name + ": "
                + e.what());

            for (auto j = i; j--;)
                modules[j].shutdown();

            return false;
        }

    return true;
}


const char* dpsoGetError(void)
{
    return lastError.c_str();
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
