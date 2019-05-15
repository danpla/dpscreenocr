
#pragma once

#include <memory>
#include <string>

#include <windows.h>


namespace dpso {
namespace backend {


std::string getLastErrorMessage();


struct WindowDestroyer {
    using pointer = HWND;

    void operator()(HWND window) const
    {
        DestroyWindow(window);
    }
};


using WindowPtr = std::unique_ptr<HWND, WindowDestroyer>;


struct DcReleaser {
    using pointer = HDC;

    explicit DcReleaser(HWND window)
        : window {window}
    {

    }

    void operator()(HDC dc) const
    {
        ReleaseDC(window, dc);
    }

    HWND window;
};


struct DcDeleter {
    using pointer = HDC;

    void operator()(HDC dc) const
    {
        DeleteDC(dc);
    }
};


template<typename FinalizerT>
using DcPtr = std::unique_ptr<HDC, FinalizerT>;


inline DcPtr<DcReleaser> getDc(HWND window)
{
    return DcPtr<DcReleaser>(GetDC(window), DcReleaser(window));
}


inline DcPtr<DcDeleter> createCompatibleDc(HDC dc)
{
    return DcPtr<DcDeleter>(CreateCompatibleDC(dc));
}


template<typename T>
struct ObjectDeleter {
    using pointer = T;

    void operator()(T object) const
    {
        DeleteObject(object);
    }
};


template<typename ObjectT>
using ObjectPtr = std::unique_ptr<ObjectT, ObjectDeleter<ObjectT>>;


// RAII for SelectObject()
struct ObjectSelector {
    ObjectSelector(HDC dc, HGDIOBJ newObject)
        : dc {dc}
        , oldObject {SelectObject(dc, newObject)}
    {

    }

    ObjectSelector(const ObjectSelector& other) = delete;
    ObjectSelector& operator=(const ObjectSelector& other) = delete;

    ObjectSelector(ObjectSelector&& other) = delete;
    ObjectSelector& operator=(ObjectSelector&& other) = delete;

    ~ObjectSelector()
    {
        if (oldObject)
            SelectObject(dc, oldObject);
    }
private:
    HDC dc;
    HGDIOBJ oldObject;
};


}
}
