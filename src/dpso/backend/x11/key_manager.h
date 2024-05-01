
#pragma once

#include <vector>

#include <X11/Xlib.h>

#include "backend/key_manager.h"
#include "backend/x11/backend_component.h"


namespace dpso::backend::x11 {


class KeyManager
    : public backend::KeyManager
    , public BackendComponent {
public:
    explicit KeyManager(Display* display);
    ~KeyManager();

    bool getIsEnabled() const override;
    void setIsEnabled(bool newIsEnabled) override;
    DpsoHotkeyAction getLastHotkeyAction() const override;

    void bindHotkey(
        const DpsoHotkey& hotkey, DpsoHotkeyAction action) override;

    int getNumBindings() const override;
    HotkeyBinding getBinding(int idx) const override;
    void removeBinding(int idx) override;

    void updateStart() override;
    void handleEvent(const XEvent& event) override;
private:
    Display* display;
    bool isEnabled{};

    struct X11HotkeyBinding {
        HotkeyBinding binding;
        KeyCode keyCode;
    };
    std::vector<X11HotkeyBinding> x11bindings;

    DpsoHotkeyAction hotkeyAction{dpsoNoHotkeyAction};

    static void changeGrab(
        Display* display,
        const X11HotkeyBinding& x11binding,
        bool grab);
};


}
