
#pragma once

#include <objbase.h>

#include <memory>
#include <utility>


namespace dpso::windows {


// Check result of CoInitializeEx() for success.
inline bool coInitSuccess(HRESULT hresult)
{
    // S_FALSE means that COM is already initialized. We should still
    // call CoUninitialize() in this case.
    return hresult == S_OK || hresult == S_FALSE;
}


// RAII for CoInitialize()
//
// COINIT_DISABLE_OLE1DDE is always added to dwCoInit.
//
// The class doesn't throw if CoInitialize() returns an error (e.g.
// RPC_E_CHANGED_MODE). Instead, it only calls CoUninitialize() from
// the destructor on success (i.e. S_OK or S_FALSE), and gives access
// to HRESULT to check for failure explicitly via coInitSuccess().
struct CoInitializer {
    explicit CoInitializer(DWORD dwCoInit)
        : hresult{
            CoInitializeEx(
                nullptr, dwCoInit | COINIT_DISABLE_OLE1DDE)}
    {
    }

    ~CoInitializer()
    {
        finalize();
    }

    CoInitializer(const CoInitializer& other) = delete;
    CoInitializer& operator=(const CoInitializer& other) = delete;

    CoInitializer(CoInitializer&& other) noexcept
    {
        *this = std::move(other);
    }

    CoInitializer& operator=(CoInitializer&& other) noexcept
    {
        if (this != &other) {
            finalize();

            hresult = other.hresult;
            moved = other.moved;
            other.moved = true;
        }

        return *this;
    }

    HRESULT getHresult() const
    {
        return hresult;
    }
private:
    HRESULT hresult{};
    bool moved{};

    void finalize()
    {
        if (!moved && coInitSuccess(hresult))
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
