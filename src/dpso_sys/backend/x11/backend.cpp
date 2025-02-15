#include <cstdlib>
#include <cstring>
#include <string>

#include <X11/Xlib.h>

#include "backend/backend.h"
#include "backend/backend_error.h"
#include "backend/x11/key_manager.h"
#include "backend/x11/screenshot.h"
#include "backend/x11/selection.h"


namespace dpso::backend {
namespace x11 {
namespace {


class Backend : public backend::Backend {
public:
    Backend();

    KeyManager& getKeyManager() override;
    Selection& getSelection() override;
    img::ImgUPtr takeScreenshot(const Rect& rect) override;

    void update() override;
private:
    std::unique_ptr<Display, decltype(&XCloseDisplay)> display;

    std::unique_ptr<KeyManager> keyManager;
    std::unique_ptr<Selection> selection;

    BackendComponent* components[2];
};


Backend::Backend()
    : display{XOpenDisplay(nullptr), XCloseDisplay}
{
    if (!display)
        throw BackendError(
            std::string("Can't connect to X display ")
            + XDisplayName(nullptr));

    keyManager = std::make_unique<KeyManager>(display.get());
    selection = std::make_unique<Selection>(display.get());

    components[0] = keyManager.get();
    components[1] = selection.get();
}


KeyManager& Backend::getKeyManager()
{
    return *keyManager;
}


Selection& Backend::getSelection()
{
    return *selection;
}


img::ImgUPtr Backend::takeScreenshot(const Rect& rect)
{
    return x11::takeScreenshot(display.get(), rect);
}


void Backend::update()
{
    for (auto* component : components)
        component->updateStart();

    XEvent event;
    while (XPending(display.get())) {
        XNextEvent(display.get(), &event);

        for (auto* component : components)
            component->handleEvent(event);
    }
}


}
}


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

    return std::make_unique<x11::Backend>();
}


}
