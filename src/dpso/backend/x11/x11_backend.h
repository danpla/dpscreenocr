
#pragma once

#include <memory>

#include <X11/Xlib.h>

#include "backend/backend.h"
#include "backend/x11/x11_key_manager.h"
#include "backend/x11/x11_selection.h"


namespace dpso {
namespace backend {


class X11Backend : public Backend {
public:
    static Backend* create();

    KeyManager& getKeyManager() override;
    Selection& getSelection() override;
    std::unique_ptr<Screenshot> takeScreenshot(const Rect& rect) override;

    void update() override;
private:
    X11Backend();

    std::unique_ptr<Display, decltype(&XCloseDisplay)> display;

    std::unique_ptr<X11KeyManager> keyManager;
    std::unique_ptr<X11Selection> selection;

    X11BackendComponent* components[2];
};


}
}
