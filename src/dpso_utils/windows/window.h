
#pragma once

#include <memory>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


namespace dpso::windows {


struct WindowDestroyer {
    using pointer = HWND;

    void operator()(HWND window) const
    {
        DestroyWindow(window);
    }
};


using WindowUPtr = std::unique_ptr<HWND, WindowDestroyer>;


}
