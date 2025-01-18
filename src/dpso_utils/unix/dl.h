
#pragma once

#include <memory>

#include <dlfcn.h>


namespace dpso::unix {


struct DlHandleCloser {
    void operator()(void* handle) const
    {
        if (handle)
            dlclose(handle);
    }
};


using DlHandleUPtr = std::unique_ptr<void, DlHandleCloser>;


}
