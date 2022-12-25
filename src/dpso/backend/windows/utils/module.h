
#pragma once

#include <memory>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


namespace dpso::windows {


struct LibraryFreer {
    using pointer = HMODULE;

    void operator()(HMODULE module) const
    {
        FreeLibrary(module);
    }
};


using ModuleUPtr = std::unique_ptr<HMODULE, LibraryFreer>;


// We use an intermediate cast to void* to silence GCC warnings about
// converting FARPROC to an incompatible function type.
#define DPSO_WIN_DLL_FN(MODULE, NAME, SIGN) \
    auto NAME ## Fn = (SIGN)(void*)GetProcAddress(MODULE, #NAME)


}

