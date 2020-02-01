
#pragma once

#include <vector>

#include <windows.h>

#include "backend/backend.h"


namespace dpso {
namespace backend {


class WindowsKeyManager : public KeyManager {
public:
    WindowsKeyManager();
    ~WindowsKeyManager();

    bool getHotkeysEnabled() const override;
    void setHotkeysEnabled(bool newHotkeysEnabled) override;
    DpsoHotkeyAction getLastHotkeyAction() const override;

    bool bindHotkey(
        const DpsoHotkey& hotkey, DpsoHotkeyAction action) override;

    int getNumBindings() const override;
    HotkeyBinding getBinding(int idx) const override;
    void removeBinding(int idx) override;

    void clearLastHotkeyAction();
    void handleWmHotkey(const MSG& msg);
private:
    std::vector<HotkeyBinding> bindings;
    bool hotkeysEnabled;
    DpsoHotkeyAction hotkeyAction;

    HotkeyBinding* findBindingByHotkey(const DpsoHotkey& hotkey);
};


}
}
