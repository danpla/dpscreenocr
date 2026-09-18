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
    KeyManager& getKeyManager() override
    {
        return keyManager;
    }

    Selection& getSelection() override
    {
        return selection;
    }

    img::ImgUPtr takeScreenshot(const Rect& rect) override
    {
        return x11::takeScreenshot(display.get(), rect);
    }

    void update() override;
private:
    DisplayUPtr display{openDisplay()};
    KeyManager keyManager{display.get()};
    Selection selection{display.get()};

    BackendComponent* components[2]{&keyManager, &selection};
};


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

    for (auto* component : components)
        component->updateEnd();
}


}


std::unique_ptr<backend::Backend> createBackend()
{
    return std::make_unique<Backend>();
}


}
