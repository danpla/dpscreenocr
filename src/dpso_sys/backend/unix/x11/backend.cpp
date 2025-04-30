#include <string>

#include <X11/Xlib.h>

#include "backend/backend.h"
#include "backend/backend_error.h"
#include "backend/unix/x11/key_manager.h"
#include "backend/unix/x11/screenshot.h"
#include "backend/unix/x11/selection.h"


namespace dpso::backend::x11 {
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


std::unique_ptr<backend::Backend> createBackend()
{
    return std::make_unique<Backend>();
}


}
