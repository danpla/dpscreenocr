
#pragma once

#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "backend/key_manager.h"


namespace dpso::backend {


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

    HotkeyBinding* findBinding(const DpsoHotkey& hotkey);
};


}
