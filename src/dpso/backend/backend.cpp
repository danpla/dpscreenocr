
#ifdef DPSO_BACKEND_UNIX_NONAPPLE
    #include "backend/x11/x11_backend.h"
    using BackendImpl = dpso::backend::X11Backend;
#else
    #error "Please choose a backend"
#endif


namespace dpso {
namespace backend {


static Backend* backend;


void init()
{
    backend = new BackendImpl();
}


void shutdown()
{
    delete backend;
}


Backend& getBackend()
{
    return *backend;
}


}
}
