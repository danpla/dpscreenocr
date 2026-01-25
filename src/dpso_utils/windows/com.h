#pragma once

#include <memory>
#include <utility>

#include <objbase.h>
#include <winerror.h>


namespace dpso::windows {


// RAII for CoInitialize()
//
// COINIT_DISABLE_OLE1DDE is always added to dwCoInit.
//
// The class doesn't throw if CoInitialize() returns an error (e.g.
// RPC_E_CHANGED_MODE). Instead, it only calls CoUninitialize() from
// the destructor on success (i.e. S_OK or S_FALSE), and gives access
// to HRESULT to explicitly check for a failure.
struct CoInitializer {
    explicit CoInitializer(DWORD dwCoInit)
        : hresult{
            CoInitializeEx(
                nullptr, dwCoInit | COINIT_DISABLE_OLE1DDE)}
        , active{SUCCEEDED(hresult)}
    {
    }

    ~CoInitializer()
    {
        finalize();
    }

    CoInitializer(const CoInitializer&) = delete;
    CoInitializer& operator=(const CoInitializer&) = delete;

    CoInitializer(CoInitializer&& other) noexcept
    {
        *this = std::move(other);
    }

    CoInitializer& operator=(CoInitializer&& other) noexcept
    {
        if (this != &other) {
            finalize();

            hresult = other.hresult;
            active = std::exchange(other.active, false);
        }

        return *this;
    }

    HRESULT getHresult() const
    {
        return hresult;
    }
private:
    HRESULT hresult{};
    bool active{};

    void finalize()
    {
        if (active)
            CoUninitialize();
    }
};


struct IUnknwonReleaser {
    void operator()(IUnknown* v) const
    {
        if (v)
            v->Release();
    }
};


template<typename T>
using CoUPtr = std::unique_ptr<T, IUnknwonReleaser>;


// Wrapper around CoCreateInstance() that returns CoUPtr.
template<typename T>
HRESULT coCreateInstance(
    REFCLSID rclsid,
    LPUNKNOWN pUnkOuter,
    DWORD dwClsContext,
    CoUPtr<T>& ptr)
{
    T* rawPtr;
    const auto hresult = CoCreateInstance(
        rclsid, pUnkOuter, dwClsContext, IID_PPV_ARGS(&rawPtr));

    ptr = CoUPtr<T>{rawPtr};
    return hresult;
}


}
