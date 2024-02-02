
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


namespace dpso::windows {


// The Handle class is a RAII for HANDLE, which also eliminates the
// need to remember whether the invalid value for this handle is null
// or INVALID_HANDLE_VALUE (this depends on which API returned the
// handle).


enum class InvalidHandleType {
    null,  // Invalid handle is NULL.
    value  // Invalid handle is INVALID_HANDLE_VALUE.
};


template<InvalidHandleType invalidHandleType>
class Handle {
public:
    Handle() = default;

    explicit Handle(HANDLE h)
        : h{h}
    {
    }

    ~Handle()
    {
        if (h != getInvalidValue())
            CloseHandle(h);
    }

    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;

    Handle(Handle&& other) noexcept
        : h{other.h}
    {
        other.h = getInvalidValue();
    }

    Handle& operator=(Handle&& other) noexcept
    {
        if (this != &other) {
            if (h != getInvalidValue())
                CloseHandle(h);

            h = other.h;
            other.h = getInvalidValue();
        }

        return *this;
    }

    explicit operator bool() const
    {
        return h != getInvalidValue();
    }

    operator HANDLE() const
    {
        return h;
    }
private:
    HANDLE h{getInvalidValue()};

    static HANDLE getInvalidValue()
    {
        switch (invalidHandleType) {
        case InvalidHandleType::null:
            return nullptr;
        case InvalidHandleType::value:
            return INVALID_HANDLE_VALUE;
        }

        return {};
    }
};


}

