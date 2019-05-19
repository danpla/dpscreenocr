
#include "backend/windows/windows_backend.h"

#include "backend/windows/execution_layer/backend_executor.h"


// We can't do anything in the main thread because its messages will
// be handed by a GUI framework. Of course we can provide a low-level
// routine to connect to an event filter provided by the framework,
// but some frameworks may not provide such filters, and I'd like to
// keep the API as high-level as possible.
//
// WindowsBackendExecutor and its proxy components do the job of
// handling the backend implementation in the background thread.
// Please note that although absolutely everything is called trough an
// executor, the only routines that must be called in the background
// are the ones that use Windows API; others don't technically need
// this.
//
// The solution is simple enough to provide the mentioned low-level
// routine to handle messages from GUI framework, since it's possible
// to choose between the background and the main thread in runtime.
// For the latter, WindowsBackend::create() can simply instantiate
// WindowsBackendImpl instead of WindowsBackendExecutor, or we can
// tell WindowsBackendExecutor to use ActionExecutor that performs
// actions in the main thread.


namespace dpso {
namespace backend {


Backend* WindowsBackend::create()
{
    return new WindowsBackendExecutor();
}


}
}
