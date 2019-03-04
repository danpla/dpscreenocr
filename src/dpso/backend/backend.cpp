
#include "backend/x11/x11_backend.h"


namespace dpso {
namespace backend {


static Backend* backend;


void init()
{
    backend = new X11Backend();
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
