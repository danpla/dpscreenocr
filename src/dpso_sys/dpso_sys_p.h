#pragma once

#include "backend/backend.h"


struct DpsoKeyManager {
    dpso::backend::KeyManager& impl;
};


struct DpsoSelection {
    dpso::backend::Selection& impl;
};


struct DpsoSys {
    std::unique_ptr<dpso::backend::Backend> backend{
        dpso::backend::Backend::create()};
    DpsoKeyManager keyManager{backend->getKeyManager()};
    DpsoSelection selection{backend->getSelection()};
};
