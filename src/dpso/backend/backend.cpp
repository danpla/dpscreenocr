
#include "backend/backend.h"


namespace dpso {
namespace backend {


static std::unique_ptr<Backend> backend;


void init()
{
    backend = Backend::create();
}


void shutdown()
{
    backend.reset();
}


Backend& getBackend()
{
    return *backend;
}


}
}
