
#include <X11/Xlib.h>

#include "backend/backend.h"
#include "backend/x11/x11_key_manager.h"
#include "backend/x11/x11_screenshot.h"
#include "backend/x11/x11_selection.h"


namespace dpso {
namespace backend {


namespace {


class X11Backend : public Backend {
public:
    X11Backend();

    KeyManager& getKeyManager() override;
    Selection& getSelection() override;
    std::unique_ptr<Screenshot> takeScreenshot(const Rect& rect) override;

    void update() override;
private:
    std::unique_ptr<Display, decltype(&XCloseDisplay)> display;

    std::unique_ptr<X11KeyManager> keyManager;
    std::unique_ptr<X11Selection> selection;

    X11BackendComponent* components[2];
};


}


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


std::unique_ptr<Screenshot> X11Backend::takeScreenshot(const Rect& rect)
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


std::unique_ptr<Backend> Backend::create()
{
    return std::unique_ptr<Backend>(new X11Backend());
}


}
}
