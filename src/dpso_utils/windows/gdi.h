
#pragma once

#include <memory>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


namespace dpso::windows {


struct DcReleaser {
    using pointer = HDC;

    explicit DcReleaser(HWND window)
        : window{window}
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
using DcUPtr = std::unique_ptr<HDC, FinalizerT>;


inline DcUPtr<DcReleaser> getDc(HWND window)
{
    return DcUPtr<DcReleaser>(GetDC(window), DcReleaser(window));
}


inline DcUPtr<DcDeleter> createCompatibleDc(HDC dc)
{
    return DcUPtr<DcDeleter>(CreateCompatibleDC(dc));
}


template<typename T>
struct ObjectDeleter {
    using pointer = T;

    void operator()(T object) const
    {
        DeleteObject(object);
    }
};


template<typename T>
using ObjectUPtr = std::unique_ptr<T, ObjectDeleter<T>>;


// RAII for SelectObject()
struct ObjectSelector {
    ObjectSelector(HDC dc, HGDIOBJ newObject)
        : dc{dc}
        , oldObject{SelectObject(dc, newObject)}
    {
    }

    ~ObjectSelector()
    {
        if (oldObject)
            SelectObject(dc, oldObject);
    }

    ObjectSelector(const ObjectSelector&) = delete;
    ObjectSelector& operator=(const ObjectSelector&) = delete;

    ObjectSelector(ObjectSelector&&) = delete;
    ObjectSelector& operator=(ObjectSelector&&) = delete;
private:
    HDC dc;
    HGDIOBJ oldObject;
};


}
