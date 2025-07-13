#include <string>

#include <X11/Xlib.h>

#include "backend/backend.h"
#include "backend/backend_error.h"
#include "backend/unix/x11/key_manager.h"
#include "backend/unix/x11/screenshot.h"
#include "backend/unix/x11/selection.h"
#include "backend/unix/x11/utils.h"


namespace dpso::backend::x11 {
namespace {


Display* openDisplay()
{
    if (auto* result = XOpenDisplay(nullptr))
        return result;

    throw BackendError{
        std::string{"Can't connect to X display "}
        + XDisplayName(nullptr)};
}


class Backend : public backend::Backend {
public:
    Backend();

    KeyManager& getKeyManager() override;
    Selection& getSelection() override;
    img::ImgUPtr takeScreenshot(const Rect& rect) override;

    void update() override;
private:
    DisplayUPtr display;

    KeyManager keyManager;
    Selection selection;

    BackendComponent* components[2];
};


Backend::Backend()
    : display{openDisplay()}
    , keyManager{display.get()}
    , selection{display.get()}
    , components{&keyManager, &selection}
{
}


KeyManager& Backend::getKeyManager()
{
    return keyManager;
}


Selection& Backend::getSelection()
{
    return selection;
}


img::ImgUPtr Backend::takeScreenshot(const Rect& rect)
{
    return x11::takeScreenshot(display.get(), rect);
}


void Backend::update()
{
    for (auto* component : components)
        component->updateStart();

    for (XEvent event; XPending(display.get());) {
        XNextEvent(display.get(), &event);

        for (auto* component : components)
            if (component->handleEvent(event))
                break;
    }
}


}


std::unique_ptr<backend::Backend> createBackend()
{
    return std::make_unique<Backend>();
}


}
