
#include "backend/x11/x11_backend.h"
#include "backend/x11/x11_screenshot.h"


namespace dpso {
namespace backend {


X11Backend::X11Backend()
    : display {XOpenDisplay(nullptr), XCloseDisplay}
{
    if (!display)
        throw BackendError(
            std::string("Can't connect to X display ")
            + XDisplayName(nullptr));

    keyManager.reset(new X11KeyManager(display.get()));
    selection.reset(new X11Selection(display.get()));

    components[0] = keyManager.get();
    components[1] = selection.get();
}


KeyManager& X11Backend::getKeyManager()
{
    return *keyManager;
}


Selection& X11Backend::getSelection()
{
    return *selection;
}


Screenshot* X11Backend::takeScreenshot(const Rect& rect)
{
    return X11Screenshot::take(display.get(), rect);
}


void X11Backend::update()
{
    for (auto* component : components)
        component->updateStart();

    XEvent event;
    while (XPending(display.get())) {
        XNextEvent(display.get(), &event);

        for (auto* component : components)
            component->handleEvent(event);
    }

    for (auto* component : components)
        component->updateEnd();
}


}
}
