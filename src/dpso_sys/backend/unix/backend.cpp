#include <cstdlib>
#include <cstring>

#include "backend/backend_error.h"
#include "backend/unix/x11/backend.h"


namespace dpso::backend {


std::unique_ptr<Backend> Backend::create()
{
    // Although X11 code on Wayland runs on top of XWayland, there
    // will still be neither global hotkeys nor screenshots since
    // Wayland doesn't support them by design.
    //
    // While an attempt to take a screenshot will fail explicitly
    // (i.e. XGrabImage() will return null), the global hotkeys
    // (XGrabKey()) will just silently do nothing. Therefore, it's
    // better not to rely on X11 failures and report an explicit error
    // as early as possible, especially for the use case when a hotkey
    // triggers a screenshot.
    const auto* xdgSessionType = std::getenv("XDG_SESSION_TYPE");
    if (xdgSessionType && std::strcmp(xdgSessionType, "wayland") == 0)
        throw BackendError(
            "Wayland is not supported. Please switch to the X11/Xorg "
            "session.");

    return x11::createBackend();
}


}
